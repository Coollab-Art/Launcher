#pragma once
#include "Cool/Task/Task.hpp"
#include "httplib.h"


class Task_FetchCompatibilityFile : public Cool::Task {
public:
    auto name() const -> std::string override { return "Fetching compatibilities info (to know which version we can upgrade to without breaking your project)"; }

private:
    void execute() override;

    auto is_quick_task() const -> bool override { return false; }
    auto needs_user_confirmation_to_cancel_when_closing_app() const -> bool override { return false; }
    void cancel() override { _cancel.store(true); }

private:
    void handle_error(httplib::Result const& res);

private:
    std::atomic<bool> _cancel{false};
};