#include "Task_InstallVersion.hpp"
#include <Cool/get_system_error.hpp>
#include "Cool/DebugOptions/DebugOptions.h"
#include "Cool/File/File.h"
#include "Cool/ImGui/ImGuiExtras.h"
#include "Cool/Log/ToUser.h"
#include "ImGuiNotify/ImGuiNotify.hpp"
#include "Version.hpp"
#include "VersionManager.hpp"
#include "httplib.h"
#include "installation_path.hpp"
#include "miniz.h"
#include "tl/expected.hpp"

static auto download_zip(std::string const& download_url, std::atomic<float>& progression, std::atomic<bool> const& cancel)
    -> tl::expected<std::string, std::string>
{
    auto cli = httplib::Client{"https://github.com"};
    // Allow the client to follow redirects
    cli.set_follow_location(true);
    // Don't cancel if we have a bad internet connection. This is done in a Task so this is non-blocking anyways
    cli.set_connection_timeout(99999h);
    cli.set_read_timeout(99999h);
    cli.set_write_timeout(99999h);

    auto res = cli.Get(download_url, [&](uint64_t current, uint64_t total) {
        progression.store(static_cast<float>(current) / static_cast<float>(total));
        return !cancel.load();
    });

    if (cancel.load())
        return "";
    if (!res)
    {
        if (Cool::DebugOptions::log_debug_warnings())
            Cool::Log::ToUser::warning("Download version", httplib::to_string(res.error()));
        return tl::make_unexpected("No Internet connection");
    }
    if (res->status != 200)
    {
        if (Cool::DebugOptions::log_debug_warnings())
            Cool::Log::ToUser::warning("Download version", fmt::format("Status code {}", std::to_string(res->status)));
        return tl::make_unexpected("Oops, our online versions provider is unavailable, please check back later");
    }

    return res->body;
}

static auto extract_zip(std::string const& zip, std::filesystem::path const& installation_path, std::atomic<float>& progression, std::atomic<bool> const& cancel)
    -> tl::expected<void, std::string>
{
    auto const file_error = [&]() {
        return tl::make_unexpected(fmt::format("Make sure you have the permission to write files in the folder \"{}\"", installation_path.parent_path()));
    };
    auto const system_error = [&]() {
        return tl::make_unexpected(Cool::get_system_error());
    };

    if (!Cool::File::create_folders_if_they_dont_exist(installation_path))
        return file_error();

    auto zip_archive = mz_zip_archive{};
    memset(&zip_archive, 0, sizeof(zip_archive));

    auto const zip_error = [&]() {
        if (Cool::DebugOptions::log_debug_warnings())
            Cool::Log::ToUser::warning("Unzip version", mz_zip_get_error_string(mz_zip_get_last_error(&zip_archive)));
        return tl::make_unexpected("An unexpected error has occurred, please try again");
    };

    if (!mz_zip_reader_init_mem(&zip_archive, zip.data(), zip.size(), 0))
        return zip_error();
    auto scope_guard = sg::make_scope_guard([&] { mz_zip_reader_end(&zip_archive); });

    auto const files_count = mz_zip_reader_get_num_files(&zip_archive);
    for (mz_uint i = 0; i < files_count; ++i)
    {
        if (cancel.load())
            break;
        progression.store(static_cast<float>(i) / static_cast<float>(files_count));

        auto file_stat = mz_zip_archive_file_stat{};
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
            return zip_error();

        if (file_stat.m_is_directory)
            continue;

        auto const full_path = installation_path / file_stat.m_filename;
        if (!Cool::File::create_folders_for_file_if_they_dont_exist(full_path))
            return file_error();

        auto file_data = std::vector<char>(file_stat.m_uncomp_size);
        if (!mz_zip_reader_extract_to_mem(&zip_archive, i, file_data.data(), file_stat.m_uncomp_size, 0))
            return zip_error();

        auto ofs = std::ofstream{full_path, std::ios::binary};
        if (!ofs)
            return system_error();
        ofs.write(file_data.data(), static_cast<std::streamsize>(file_data.size()));
        if (ofs.fail())
            return system_error();
    }

    return {};
}

static auto make_file_executable(std::filesystem::path const& path) -> tl::expected<void, std::string>
{
#if defined(__linux__) || defined(__APPLE__)
    std::string const command = fmt::format("chmod u+x \"{}\" 2>&1", path); // "2>&1" redirects stderr to stdout
    // Open a pipe to capture the output of the command
    FILE* const pipe = popen(command.c_str(), "r");
    if (!pipe)
    {
        if (Cool::DebugOptions::log_debug_warnings())
            Cool::Log::ToUser::warning("Make file executable", "Failed to open command pipe");
        return tl::make_unexpected(fmt::format("Make sure you have the permission to edit the file \"{}\"", path));
    }

    auto error_message = ""s;
    {
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
            error_message += buffer;
    }

    if (pclose(pipe) != 0)
    {
        if (Cool::DebugOptions::log_debug_warnings())
            Cool::Log::ToUser::warning("Make file executable", error_message);
        return tl::make_unexpected(fmt::format("Make sure you have the permission to edit the file \"{}\"", path));
    }
#else
    std::ignore = path;
#endif
    return {};
}

Task_InstallVersion::Task_InstallVersion(VersionName name, std::string download_url)
    : _name{std::move(name)}
    , _download_url{std::move(download_url)}
{
    _notification_id = ImGuiNotify::send({
        .type                 = ImGuiNotify::Type::Info,
        .title                = fmt::format("Installing {}", _name.as_string()),
        .custom_imgui_content = [data = _data]() {
            Cool::ImGuiExtras::disabled_if(data->cancel.load(), "", [&]() {
                ImGui::ProgressBar(data->download_progress.load() * 0.9f + 0.1f * data->extraction_progress.load());
                if (ImGui::Button("Cancel"))
                    data->cancel.store(true);
            });
        },
        .duration    = std::nullopt,
        .is_closable = false,
    });
}

void Task_InstallVersion::on_success()
{
    version_manager().set_installation_status(_name, InstallationStatus::Installed);
    ImGuiNotify::change(
        _notification_id,
        {
            .type    = ImGuiNotify::Type::Success,
            .title   = fmt::format("Installed {}", _name.as_string()),
            .content = "Success",
        }
    );
}

void Task_InstallVersion::on_version_not_installed()
{
    version_manager().set_installation_status(_name, InstallationStatus::NotInstalled);
    Cool::File::remove_folder(installation_path(_name)); // Cleanup any files that we might have started to extract from the zip
}

void Task_InstallVersion::on_cancel()
{
    on_version_not_installed();
    ImGuiNotify::close_immediately(_notification_id);
}

void Task_InstallVersion::on_error(std::string const& error_message)
{
    on_version_not_installed();
    ImGuiNotify::change(
        _notification_id,
        {
            .type     = ImGuiNotify::Type::Error,
            .title    = fmt::format("Installation failed ({})", _name.as_string()),
            .content  = error_message,
            .duration = std::nullopt,
        }
    );
}

void Task_InstallVersion::do_work()
{
    version_manager().set_installation_status(_name, InstallationStatus::Installing); // TODO(Launcher) should be done in constructor

    auto const zip = download_zip(_download_url, _data->download_progress, _data->cancel);
    if (_data->cancel.load())
    {
        on_cancel();
        return;
    }
    if (!zip.has_value())
    {
        on_error(zip.error());
        return;
    }
    {
        auto const success = extract_zip(*zip, installation_path(_name), _data->extraction_progress, _data->cancel);
        if (_data->cancel.load())
        {
            on_cancel();
            return;
        }
        if (!success.has_value())
        {
            on_error(success.error());
            return;
        }
    }
    {
        auto const success = make_file_executable(executable_path(_name));
        if (!success.has_value())
        {
            on_error(success.error());
            return;
        }
    }

    on_success();
}
