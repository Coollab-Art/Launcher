#pragma once
#include "Cool/Task/TaskWithProgressBar.hpp"
#include "ImGuiNotify/ImGuiNotify.hpp"
#include "VersionName.hpp"

class Task_InstallVersion : public Cool::TaskWithProgressBar {
public:
    /// Will install the latest version if we pass nullopt
    explicit Task_InstallVersion(std::optional<VersionName> version_name = {})
        : Cool::TaskWithProgressBar{fmt::format("Installing {}", version_name ? version_name->as_string_pretty() : "latest version")}
        , _version_name{std::move(version_name)}
    {}

private:
    void on_submit() override;
    auto execute() -> Cool::TaskCoroutine override;
    void cleanup_impl(bool has_been_canceled) override;

    auto needs_user_confirmation_to_cancel_when_closing_app() const -> bool override { return true; }
    auto notification_when_submitted() const -> ImGuiNotify::Notification override;
    auto notification_after_execution_completes() const -> ImGuiNotify::Notification override;
    auto extra_imgui_below_progress_bar() const -> std::function<void()> override;

private:
    std::optional<VersionName> _version_name{};
    std::optional<std::string> _download_url{};
    std::optional<std::string> _changelog_url{};

    std::optional<std::string> _error_message{};
};
