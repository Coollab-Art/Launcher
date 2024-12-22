#pragma once
#include "Cool/Task/Task.hpp"
#include "ImGuiNotify/ImGuiNotify.hpp"
#include "VersionName.hpp"

class Task_InstallVersion : public Cool::Task {
public:
    Task_InstallVersion(VersionName version_name, std::string download_url, ImGuiNotify::NotificationId notification_id);

    void do_work() override;
    void cancel() override { _data->cancel.store(true); }
    auto needs_user_confirmation_to_cancel_when_closing_app() const -> bool override { return true; }
    auto name() const -> std::string override { return fmt::format("Installing {}", _version_name.as_string()); }

private:
    void on_success();
    void on_cancel();
    void on_error(std::string const& error_message);
    void on_version_not_installed();

private:
    VersionName                 _version_name;
    std::string                 _download_url{};
    ImGuiNotify::NotificationId _notification_id{};

    struct DataSharedWithNotification {
        std::atomic<bool>  cancel{false};
        std::atomic<float> download_progress{0.f};
        std::atomic<float> extraction_progress{0.f};
    };
    std::shared_ptr<DataSharedWithNotification> _data{std::make_shared<DataSharedWithNotification>()};
};
