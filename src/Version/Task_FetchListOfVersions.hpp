#pragma once
#include "Cool/Task/Task.hpp"
#include "ImGuiNotify/ImGuiNotify.hpp"
#include "httplib.h"

class Task_FetchListOfVersions : public Cool::Task {
public:
    explicit Task_FetchListOfVersions(std::optional<ImGuiNotify::NotificationId> warning_notification_id = {})
        : _warning_notification_id{warning_notification_id}
    {}

    auto name() const -> std::string override { return "Fetching the list of versions that are available online"; }

private:
    void execute() override;

    auto is_quick_task() const -> bool override { return false; }
    auto needs_user_confirmation_to_cancel_when_closing_app() const -> bool override { return false; }
    void cancel() override { _cancel.store(true); }

private:
    void handle_error(httplib::Result const& res);

private:
    std::atomic<bool>                          _cancel{false};
    std::optional<ImGuiNotify::NotificationId> _warning_notification_id{};
};