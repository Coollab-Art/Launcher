#pragma once
#include "Cool/Task/Task.hpp"
#include "httplib.h"

class Task_FetchCompatibilityFile : public Cool::Task {
public:
    Task_FetchCompatibilityFile()
        : Cool::Task{"Fetching compatibilities info (to know which version we can upgrade to without breaking your project)"}
    {}

private:
    auto execute() -> Cool::TaskCoroutine override;

    auto needs_user_confirmation_to_cancel_when_closing_app() const -> bool override { return false; }

private:
    void handle_error(httplib::Result const& res);
};