#pragma once
#include "Cool/Task/Task.hpp"

class Task_UninstallUnusedVersions : public Cool::Task {
public:
    Task_UninstallUnusedVersions() = default;

private:
    void on_submit() override{};
    void execute() override;
    void cleanup(bool) override{};

    std::string name() const override{return 0;};

    bool is_quick_task() const override{return 0;};

    void cancel() override{};

    bool needs_user_confirmation_to_cancel_when_closing_app() const override{return 0;};
};