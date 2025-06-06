#pragma once
#include <ImGuiNotify/ImGuiNotify.hpp>
#include <reg/src/generate_uuid.hpp>
#include "Cool/Task/Task.hpp"
#include "ProjectToOpenOrCreate.hpp"
#include "VersionRef.hpp"

/// NB: the version must be installed before execute() is called
class Task_LaunchVersion : public Cool::Task {
public:
    explicit Task_LaunchVersion(VersionRef version_ref, ProjectToOpenOrCreate project_to_open_or_create);

private:
    void on_submit() override;
    auto execute() -> Cool::TaskCoroutine override;
    void cleanup_impl(bool has_been_canceled) override;

    auto needs_user_confirmation_to_cancel_when_closing_app() const -> bool override { return false; }

private:
    VersionRef                  _version_ref;
    ProjectToOpenOrCreate       _project_to_open_or_create{};
    ImGuiNotify::NotificationId _notification_id{};
    std::string                 _error_message{""};
};