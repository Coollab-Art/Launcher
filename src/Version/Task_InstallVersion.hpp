#pragma once
#include "Cool/Task/Task.hpp"
#include "ImGuiNotify/ImGuiNotify.hpp"
#include "VersionName.hpp"

class Task_InstallVersion : public Cool::Task {
public:
    Task_InstallVersion(VersionName name, std::string download_url);
    ~Task_InstallVersion() override;
    Task_InstallVersion(Task_InstallVersion const&)                = delete;
    Task_InstallVersion& operator=(Task_InstallVersion const&)     = delete;
    Task_InstallVersion(Task_InstallVersion&&) noexcept            = delete;
    Task_InstallVersion& operator=(Task_InstallVersion&&) noexcept = delete;

    void do_work() override;
    void cancel() override { _data->cancel.store(true); }
    auto needs_user_confirmation_to_cancel_when_closing_app() const -> bool override { return true; }

private:
    VersionName                 _name;
    std::string                 _download_url{};
    ImGuiNotify::NotificationId _notification_id{};

    struct DataSharedWithNotification {
        std::atomic<bool>  cancel{false};
        std::atomic<float> download_progress{0.f};
        std::atomic<float> extraction_progress{0.f};
    };
    std::shared_ptr<DataSharedWithNotification> _data{std::make_shared<DataSharedWithNotification>()};
};
