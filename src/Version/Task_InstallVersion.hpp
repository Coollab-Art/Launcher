#pragma once
#include "Cool/Task/TaskWithProgressBar.hpp"
#include "ImGuiNotify/ImGuiNotify.hpp"
#include "VersionName.hpp"

class Task_InstallVersion : public Cool::TaskWithProgressBar {
public:
    Task_InstallVersion() = default; // Will install the latest version, because we don't specify a version name
    explicit Task_InstallVersion(VersionName version_name)
        : _version_name{std::move(version_name)}
    {}
    auto name() const -> std::string override { return fmt::format("Installing {}", _version_name ? _version_name->as_string() : "latest version"); }

private:
    void on_submit() override;
    void execute() override;
    void cleanup(bool has_been_canceled) override;

    auto needs_user_confirmation_to_cancel_when_closing_app() const -> bool override { return true; }
    auto text_in_notification_while_waiting_to_execute() const -> std::string override;
    auto notification_after_execution_completes() const -> ImGuiNotify::Notification override;

private:
    std::optional<VersionName> _version_name{};
    std::optional<std::string> _download_url{};

    std::optional<std::string> _error_message{};
};
