#pragma once
#include "Cool/Task/Task.hpp"
#include "ImGuiNotify/ImGuiNotify.hpp"
#include "httplib.h"

class Task_FetchListOfVersions : public Cool::Task {
public:
    explicit Task_FetchListOfVersions(std::optional<ImGuiNotify::NotificationId> warning_notification_id = {})
        : Cool::Task{"Fetching the list of versions that are available online"}
        , _warning_notification_id{warning_notification_id}
    {}

private:
    auto execute() -> Cool::TaskCoroutine override;
    auto needs_user_confirmation_to_cancel_when_closing_app() const -> bool override { return false; }

    void handle_error(httplib::Result const& res);

private:
    std::optional<ImGuiNotify::NotificationId> _warning_notification_id{};
};