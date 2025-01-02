#pragma once
#include <ImGuiNotify/ImGuiNotify.hpp>
#include <reg/src/generate_uuid.hpp>
#include "Cool/File/File.h"
#include "Cool/Task/Task.hpp"
#include "VersionRef.hpp"

/// NB: the version must be installed before execute() is called
class Task_LaunchVersion : public Cool::Task {
public:
    explicit Task_LaunchVersion(VersionRef version_ref, std::optional<std::filesystem::path> project_file_path);

    auto name() const -> std::string override { return fmt::format("Launching {}", _project_file_path ? fmt::format("\"{}\"", Cool::File::file_name_without_extension(*_project_file_path)) : "a new project"); }

private:
    void on_submit() override;
    void execute() override;
    void cleanup(bool has_been_canceled) override;

    auto is_quick_task() const -> bool override { return true; }
    void cancel() override {}
    auto needs_user_confirmation_to_cancel_when_closing_app() const -> bool override { return false; }

private:
    VersionRef                           _version_ref;
    std::optional<std::filesystem::path> _project_file_path{};
    ImGuiNotify::NotificationId          _notification_id{};
    std::string                          _error_message{""};
};