#include "Task_InstallVersion.hpp"
#include <imgui.h>
#include <Cool/get_system_error.hpp>
#include "Cool/File/File.h"
#include "Cool/ImGui/markdown.h"
#include "ImGuiNotify/ImGuiNotify.hpp"
#include "Version.hpp"
#include "VersionManager.hpp"
#include "httplib.h"
#include "installation_path.hpp"
#include "make_http_request.hpp"
#include "mz.h"
#include "mz_strm.h"
#include "mz_strm_mem.h"
#include "mz_zip.h"
#include "mz_zip_rw.h"
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

#if !defined(__linux__) // This function is not used on Linux
static auto minizip_error_string(int32_t code) -> std::string
{
    switch (code)
    {
    case MZ_OK: return "Success";
    case MZ_MEM_ERROR: return "Memory error";
    case MZ_PARAM_ERROR: return "Invalid parameter";
    case MZ_FORMAT_ERROR: return "ZIP format error";
    case MZ_EXIST_ERROR: return "File already exists";
    case MZ_OPEN_ERROR: return "Cannot open file";
    case MZ_CLOSE_ERROR: return "Cannot close file";
    case MZ_READ_ERROR: return "Read error";
    case MZ_WRITE_ERROR: return "Write error";
    case MZ_CRC_ERROR: return "CRC mismatch";
    default: return fmt::format("Unknown error ({})", code);
    }
}
#endif

static auto extract_zip(std::string const& zip, VersionName const& version_name, std::function<bool()> const& wants_to_cancel)
    -> tl::expected<void, std::string>
{
#if defined(__linux__)
    // On Linux we don't have a zip, just an AppImage that is already ready to use
    Cool::File::set_content(executable_path(version_name), zip);
    std::ignore = wants_to_cancel;
#else
    auto const file_error = [&]() {
        return tl::make_unexpected(fmt::format("Make sure you have the permission to write files in the folder \"{}\"", installation_path(version_name).parent_path()));
    };
    auto const zip_error = [](std::string const& debug_error_message) {
        Cool::Log::internal_warning("Unzip version", debug_error_message);
        return tl::make_unexpected("An unexpected error has occurred, please try again");
    };

    if (!Cool::File::create_folders_if_they_dont_exist(installation_path(version_name)))
        return file_error();

    void* reader = mz_zip_reader_create();
    if (!reader)
        return zip_error("Failed to initialize zip reader");
    auto const scope_guard = sg::make_scope_guard([&] { mz_zip_reader_delete(&reader); });

    void* stream_mem = mz_stream_mem_create();
    if (!stream_mem)
        return zip_error("Failed to initialize stream memory");
    auto const scope_guard2 = sg::make_scope_guard([&] { mz_stream_mem_delete(&stream_mem); });

    mz_stream_mem_set_buffer(stream_mem, reinterpret_cast<void*>(const_cast<char*>(zip.data())), static_cast<int32_t>(zip.size())); // NOLINT(*const-cast, *reinterpret-cast)
    {
        auto const res = mz_stream_mem_seek(stream_mem, 0, MZ_SEEK_SET);
        if (res != MZ_OK)
            return zip_error(fmt::format("Failed to seek stream: {}", minizip_error_string(res)));
    }
    {
        auto const res = mz_zip_reader_open(reader, stream_mem);
        if (res != MZ_OK)
            return zip_error(fmt::format("Failed to open zip from memory: {}", minizip_error_string(res)));
    }
    auto const scope_guard3 = sg::make_scope_guard([&] { mz_zip_reader_close(reader); });

    while (mz_zip_reader_goto_next_entry(reader) == MZ_OK)
    {
        if (wants_to_cancel())
            return {}; // No error

        {
            auto const res = mz_zip_reader_entry_open(reader);
            if (res != MZ_OK)
                return zip_error(fmt::format("Failed to open zip entry: {}", minizip_error_string(res)));
        }
        auto const scope_guard4 = sg::make_scope_guard([&] { mz_zip_reader_entry_close(reader); });

        mz_zip_file* file_info{};
        {
            auto const res = mz_zip_reader_entry_get_info(reader, &file_info);
            if (res != MZ_OK || file_info == nullptr || file_info->filename == nullptr)
                return zip_error(fmt::format("Failed to get entry info: {}", minizip_error_string(res)));
        }

        auto const full_path = installation_path(version_name) / file_info->filename;
        if (!Cool::File::create_folders_for_file_if_they_dont_exist(full_path))
            return file_error();

        {
            auto const res = mz_zip_reader_entry_save_file(reader, full_path.string().c_str());
            if (res != MZ_OK)
                return zip_error(fmt::format("Failed to extract file \"{}\": {}", file_info->filename, minizip_error_string(res)));
        }
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

void Task_InstallVersion::execute()
{
    // Find version name and/or download url if necessary
    // We need to do this in execute, because we might have been waiting for FetchListOfVersions to finish, so we didn't have access to the download url before that point
    if (!_version_name.has_value()) // If we don't give us a version name, we will install the latest version (this happens when we want to install the latest version, but haven't fetched the list of versions yet so we can't know its name when creating the install task)
    {
        auto const* const version = version_manager().latest_version(false /*filter_experimental_versions*/);
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
        auto const* const version = version_manager().find(*_version_name, false /*filter_experimental_versions*/);
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
    auto const zip = download_zip(*_download_url, [&](float progress) { set_progress(progress * 0.99f); }, [&]() { return cancel_requested(); });
    if (cancel_requested())
        return;
    if (!zip.has_value())
    {
        _error_message = zip.error();
        return;
    }

    { // Extract zip
        auto const success = extract_zip(*zip, *_version_name, [&]() { return cancel_requested(); });
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
