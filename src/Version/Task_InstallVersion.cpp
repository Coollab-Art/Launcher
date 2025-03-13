#include "Task_InstallVersion.hpp"
#include <Cool/get_system_error.hpp>
#include "Cool/DebugOptions/DebugOptions.h"
#include "Cool/File/File.h"
#include "Cool/ImGui/markdown.h"
#include "ImGuiNotify/ImGuiNotify.hpp"
#include "Version.hpp"
#include "VersionManager.hpp"
#include "httplib.h"
#include "installation_path.hpp"
#include "make_http_request.hpp"
#include "miniz.h"
#include "tl/expected.hpp"

static auto download_zip(std::string const& download_url, std::function<void(float)> const& set_progress, std::function<bool()> const& wants_to_cancel)
    -> tl::expected<std::string, std::string>
{
    auto res = make_http_request(download_url, [&](uint64_t current, uint64_t total) {
        set_progress(static_cast<float>(current) / static_cast<float>(total));
        return !wants_to_cancel();
    });

    if (wants_to_cancel())
        return "";
    if (!res)
    {
        Cool::Log::internal_warning("Download version", httplib::to_string(res.error()));
        return tl::make_unexpected("No Internet connection");
    }
    if (res->status != 200)
    {
        Cool::Log::internal_warning("Download version", fmt::format("Status code {}", std::to_string(res->status)));
        return tl::make_unexpected("Oops, our online versions provider is unavailable, please check back later");
    }

    return res->body;
}

static auto extract_zip(std::string const& zip, VersionName const& version_name, std::function<void(float)> const& set_progress, std::function<bool()> const& wants_to_cancel)
    -> tl::expected<void, std::string>
{
#if defined(__linux__)
    // On Linux we don't have a zip, just an AppImage that is already ready to use
    Cool::File::set_content(executable_path(version_name), zip);
    std::ignore = set_progress;
    std::ignore = wants_to_cancel;
#else
    auto const file_error = [&]() {
        return tl::make_unexpected(fmt::format("Make sure you have the permission to write files in the folder \"{}\"", installation_path(version_name).parent_path()));
    };
    auto const system_error = [&]() {
        return tl::make_unexpected(Cool::get_system_error());
    };

    if (!Cool::File::create_folders_if_they_dont_exist(installation_path(version_name)))
        return file_error();

    auto zip_archive = mz_zip_archive{};
    memset(&zip_archive, 0, sizeof(zip_archive));

    auto const zip_error = [&]() {
        Cool::Log::internal_warning("Unzip version", mz_zip_get_error_string(mz_zip_get_last_error(&zip_archive)));
        return tl::make_unexpected("An unexpected error has occurred, please try again");
    };

    if (!mz_zip_reader_init_mem(&zip_archive, zip.data(), zip.size(), 0))
        return zip_error();
    auto scope_guard = sg::make_scope_guard([&] { mz_zip_reader_end(&zip_archive); });

    auto const files_count = mz_zip_reader_get_num_files(&zip_archive);
    for (mz_uint i = 0; i < files_count; ++i)
    {
        if (wants_to_cancel())
            break;
        set_progress(static_cast<float>(i) / static_cast<float>(files_count));

        auto file_stat = mz_zip_archive_file_stat{};
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
            return zip_error();

        if (file_stat.m_is_directory)
            continue;

        auto const full_path = installation_path(version_name) / file_stat.m_filename;
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
#endif
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
        Cool::Log::internal_warning("Make file executable", "Failed to open command pipe");
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
        Cool::Log::internal_warning("Make file executable", error_message);
        return tl::make_unexpected(fmt::format("Make sure you have the permission to edit the file \"{}\"", path));
    }
#else
    std::ignore = path;
#endif
    return {};
}

void Task_InstallVersion::on_submit()
{
    Cool::TaskWithProgressBar::on_submit();

    if (_version_name.has_value())
        version_manager().set_installation_status(*_version_name, InstallationStatus::Installing);
}

auto Task_InstallVersion::text_in_notification_while_waiting_to_execute() const -> std::string
{
    if (version_manager().status_of_fetch_list_of_versions() == Status::Completed)
        return Cool::TaskWithProgressBar::text_in_notification_while_waiting_to_execute();
    return "Waiting to connect to the Internet";
}

auto Task_InstallVersion::notification_after_execution_completes() const -> ImGuiNotify::Notification
{
    if (!_error_message.has_value())
    {
        auto notif                 = Cool::TaskWithProgressBar::notification_after_execution_completes();
        notif.custom_imgui_content = extra_imgui_below_progress_bar();
        return notif;
    }

    return ImGuiNotify::Notification{
        .type     = ImGuiNotify::Type::Error,
        .title    = name(),
        .content  = *_error_message,
        .duration = std::nullopt,
    };
}

auto Task_InstallVersion::extra_imgui_below_progress_bar() const -> std::function<void()>
{
    if (!_changelog_url.has_value())
        return []() {
        };

    return [url = *_changelog_url]() {
        Cool::ImGuiExtras::markdown(fmt::format("[View changes added in this version]({})", url));
    };
}

void Task_InstallVersion::cleanup(bool has_been_canceled)
{
    Cool::TaskWithProgressBar::cleanup(has_been_canceled);

    if (!_version_name.has_value())
        return;

    if (has_been_canceled || _error_message.has_value())
    {
        version_manager().set_installation_status(*_version_name, InstallationStatus::NotInstalled);
        Cool::File::remove_folder(installation_path(*_version_name)); // Cleanup any files that we might have started to extract from the zip
    }
    else
    {
        version_manager().set_installation_status(*_version_name, InstallationStatus::Installed);
    }
}

static auto proportion_of_the_install_time_represented_by_download() -> float
{
#if defined(__linux__)
    return 1.f; // On Linux there is no zip to extract, so downloading represents 100% of the install time
#else
    return 0.9f;
#endif
}

void Task_InstallVersion::execute()
{
    // Find version name and/or download url if necessary
    // We need to do this in execute, because we might have been waiting for FetchListOfVersions to finish, so we didn't have access to the download url before that point
    if (!_version_name.has_value()) // If we don't give us a version name, we will install the latest version (this happens when we want to install the latest version, but haven't fetched the list of versions yet so we can't know its name when creating the install task)
    {
        auto const* const version = version_manager().latest_version();
        if (!version || !version->download_url.has_value())
        {
            _error_message = "Didn't find any version to install";
            return;
        }
        _version_name  = version->name;
        _download_url  = version->download_url;
        _changelog_url = version->changelog_url;
        version_manager().set_installation_status(*_version_name, InstallationStatus::Installing);
    }
    if (!_download_url.has_value())
    {
        auto const* const version = version_manager().find(*_version_name);
        if (!version || !version->download_url.has_value())
        {
            _error_message = "This version is not available online";
            return;
        }
        _download_url  = version->download_url;
        _changelog_url = version->changelog_url;
    }

    TaskWithProgressBar::change_notification_when_execution_starts(); // Must be done after finding the _changelog_url, because this will call extra_imgui_below_progress_bar(), which needs _changelog_url

    // Download zip
    float const dl_prop = proportion_of_the_install_time_represented_by_download();

    auto const zip = download_zip(*_download_url, [&](float progress) { set_progress(dl_prop * progress); }, [&]() { return cancel_requested(); });
    if (cancel_requested())
        return;
    if (!zip.has_value())
    {
        _error_message = zip.error();
        return;
    }

    { // Extract zip
        auto const success = extract_zip(*zip, *_version_name, [&](float progress) { set_progress(dl_prop + (1.f - dl_prop) * progress); }, [&]() { return cancel_requested(); });
        if (cancel_requested())
            return;
        if (!success.has_value())
        {
            _error_message = success.error();
            return;
        }
    }

    { // Make file executable
        auto const success = make_file_executable(executable_path(*_version_name));
        if (!success.has_value())
        {
            _error_message = success.error();
            return;
        }
    }
}
