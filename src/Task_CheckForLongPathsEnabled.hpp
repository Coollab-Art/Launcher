#pragma once
#include "Cool/Task/Task.hpp"
#include "ImGuiNotify/ImGuiNotify.hpp"

#if defined(_WIN32)
class Task_CheckForLongPathsEnabled : public Cool::Task {
public:
    explicit Task_CheckForLongPathsEnabled(std::optional<ImGuiNotify::NotificationId> notification_id = {})
        : _notification_id{notification_id}
    {}

    void do_work() override;
    void cancel() override {}
    auto needs_user_confirmation_to_cancel_when_closing_app() const -> bool override { return false; }
    auto name() const -> std::string override { return "Checking if Long Paths are enabled in the Windows settings"; }

private:
    std::optional<ImGuiNotify::NotificationId> _notification_id{};
};
#endif
