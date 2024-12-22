#pragma once
#include "Cool/Task/Task.hpp"
#include "ImGuiNotify/ImGuiNotify.hpp"
#include "VersionName.hpp"

class Task_WaitForDownloadUrlToInstallVersion : public Cool::Task {
public:
    explicit Task_WaitForDownloadUrlToInstallVersion(VersionName version_name, ImGuiNotify::NotificationId = {}, std::shared_ptr<std::atomic<bool>> = std::make_unique<std::atomic<bool>>(false));

    void do_work() override;
    void cancel() override
    {
        _cancel->store(true);
        ImGuiNotify::close_immediately(_notification_id);
    }
    auto needs_user_confirmation_to_cancel_when_closing_app() const -> bool override { return true; }
    auto name() const -> std::string override { return fmt::format("Installing {}", _version_name.as_string()); }

private:
    VersionName                        _version_name;
    ImGuiNotify::NotificationId        _notification_id{};
    std::shared_ptr<std::atomic<bool>> _cancel;
};
