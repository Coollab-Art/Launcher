#include "Task_FetchListOfVersions.hpp"
#include "Cool/DebugOptions/DebugOptions.h"
#include "Cool/Log/ToUser.h"
#include "Cool/Task/TaskManager.hpp"
#include "Status.hpp"
#include "VersionManager.hpp"
#include "make_http_request.hpp"
#include "nlohmann/json.hpp"

static auto asset_name_for_current_os() -> std::string_view
{
#if defined(_WIN32)
    return "Coollab-Windows.zip"sv;
#elif defined(__linux__)
    return "Coollab.AppImage"sv;
#elif defined(__APPLE__)
    return "Coollab-MacOS.zip"sv;
#else
#error "Unsupported platform"
#endif
}

void Task_FetchListOfVersions::execute()
{
    auto const res = make_http_request("https://api.github.com/repos/CoolLibs/Lab/releases", [&](uint64_t, uint64_t) {
        return !_cancel.load();
    });

    if (!res || res->status != 200)
    {
        handle_error(res);
        return;
    }

    try
    {
        auto const json_response = nlohmann::json::parse(res->body);
        for (auto const& version_json : json_response)
        {
            if (_cancel.load())
                return;

            try
            {
                if (version_json.at("draft") == true)
                    continue;

                auto const version_name = VersionName::from(version_json.at("name"));
                if (!version_name.has_value()) // This will ignore all the old Beta versions, which is what we want because they are not compatible with the launcher
                    continue;

                for (auto const& asset : version_json.at("assets"))
                {
                    if (asset.at("name") != asset_name_for_current_os())
                        continue;
                    version_manager().set_download_url(*version_name, asset.at("browser_download_url"));
                    break;
                }

                version_manager().set_changelog_url(*version_name, fmt::format("https://github.com/CoolLibs/Lab/blob/{}/changelog.md", std::string{version_json.at("tag_name")}));
            }
            catch (std::exception const& e)
            {
                if (Cool::DebugOptions::log_internal_warnings())
                    Cool::Log::ToUser::error("Fetch list of versions", e.what());
            }
        }
    }
    catch (std::exception const& e)
    {
        if (Cool::DebugOptions::log_internal_warnings())
            Cool::Log::ToUser::error("Fetch list of versions", e.what());
    }

    version_manager().on_finished_fetching_list_of_versions();

    if (_warning_notification_id.has_value())
        ImGuiNotify::close_immediately(*_warning_notification_id);
}

void Task_FetchListOfVersions::handle_error(httplib::Result const& res)
{
    if (Cool::DebugOptions::log_internal_warnings())
    {
        Cool::Log::ToUser::warning(
            "Fetch list of versions",
            !res ? httplib::to_string(res.error())
                 : fmt::format("Status code {}", std::to_string(res->status))
        );
    }

    auto message              = std::optional<std::string>{};
    auto duration_until_reset = std::chrono::seconds{};

    if (res && res->status == 403)
    {
        auto const it = res->headers.find("X-RateLimit-Reset");
        if (it != res->headers.end())
        {
            try
            {
                auto const reset_time_unix   = time_t{std::stoi(it->second)};
                auto const current_time_unix = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                duration_until_reset         = std::chrono::seconds{reset_time_unix - current_time_unix};
                auto const minutes           = duration_cast<std::chrono::minutes>(duration_until_reset);
                auto const seconds           = duration_until_reset - minutes;
                message                      = fmt::format("You need to wait {}\nYou opened the launcher more than 60 times in 1 hour, which is the maximum number of requests we can make to our online service to check for available versions", minutes.count() == 0 ? fmt::format("{}s", seconds.count()) : fmt::format("{}m {}s", minutes.count(), seconds.count()));
            }
            catch (...) // NOLINT(*bugprone-empty-catch)
            {
            }
        }
    }

    auto const notification = ImGuiNotify::Notification{
        .type     = ImGuiNotify::Type::Warning,
        .title    = "Failed to check for new versions online",
        .content  = !res ? "No Internet connection" : message.value_or("Oops, our online versions provider is unavailable"),
        .duration = std::nullopt,
    };

    if (!_warning_notification_id)
        _warning_notification_id = ImGuiNotify::send(notification);
    else
        ImGuiNotify::change(*_warning_notification_id, notification);

    if (!res || message) // Only retry if we failed because we don't have an Internet connection, or because we hit the max number of requests to Github. There is no point in retrying if the service is unavailable, it's probably not gonna get fixed soon, and if we make too many requests to their API, Github will block us
        Cool::task_manager().submit(after(message ? duration_until_reset : 1s), std::make_shared<Task_FetchListOfVersions>(_warning_notification_id));
    else
        version_manager()._status_of_fetch_list_of_versions.store(Status::Canceled);
}