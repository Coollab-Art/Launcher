#pragma once
#include "Cool/AppManager/IApp.h"
#include "Cool/DebugOptions/DebugOptions.h"
#include "Cool/DebugOptions/DebugOptionsManager.h"
#include "Cool/View/ViewsManager.h"
#include "Cool/Window/Window.h"
#include "Cool/Window/WindowManager.h"
#include "DebugOptions/DebugOptions.hpp"
#include "Project/ProjectManager.hpp"
#include "Version/VersionRef.hpp"

using DebugOptionsManager = Cool::DebugOptionsManager<Launcher::DebugOptions, Cool::DebugOptions>;

class App : public Cool::IApp {
public:
    App(Cool::WindowManager& windows, Cool::ViewsManager& /* views */);

    void init() override;
    void update() override;
    void imgui_windows() override;
    void imgui_menus() override;
    auto wants_to_show_menu_bar() const -> bool override { return true; }
    void on_shutdown() override;

private:
    void open_external_project();
    void open_new_project();
    void launch(Project const& project);
    void launch(std::filesystem::path const& project_file_path);

private:
    ProjectManager        _project_manager{};
    VersionRef            _version_to_use_for_new_project{LatestInstalledVersion{}};
    Cool::Window&         _window; // NOLINT(*avoid-const-or-ref-data-members)
    std::filesystem::path _projects_folder{};

private:
    void save_to_json(nlohmann::json& json) const override
    {
        Cool::json_set(json, "Projects folder", _projects_folder);
    }
    void load_from_json(nlohmann::json const& json) override
    {
        Cool::json_get(json, "Projects folder", _projects_folder);
    }
};