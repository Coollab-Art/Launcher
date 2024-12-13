#include "App.hpp"
#include "Cool/ImGui/ImGuiExtras_dropdown.hpp"
#include "Cool/ImGui/markdown.h"
#include "Cool/Task/TaskManager.hpp"
#include "ImGuiNotify/ImGuiNotify.hpp"
#include "Task_CheckForLongPathsEnabled.hpp"
#include "Version/VersionName.hpp"

App::App(Cool::WindowManager& windows, Cool::ViewsManager& /* views */)
    : _version_to_use_for_new_project{_version_manager.latest()}
    , _window{windows.main_window()}
{
#if defined(_WIN32)
    if (_project_manager.has_some_projects()) // Don't show it the first time users open the launcher after installing it, because we don't want to scare them with something that might look like a virus
    {
        Cool::task_manager().run_small_task_in(500ms, // Small delay to make sure users see it pop up and it draws their attention
                                               std::make_shared<Task_CheckForLongPathsEnabled>());
    }
#endif
}

void App::imgui_windows()
{
    { // Versions
        ImGui::Begin("Versions");
        _version_manager.imgui();
        ImGui::End();
    }

    { // New Project
        ImGui::Begin("New Project");
        if (ImGui::Button("New Project")) // TODO(Launcher) Nice big blue button // TODO(Launcher) Should always be on top, even when we scroll through the projects
        {
            _version_to_use_for_new_project->launch(); // TODO(Launcher) Handle when no version
            _window.close();
        }
        ImGui::SameLine();
        // TODO(Launcher) dropdown options "latest", "latest installed"
        // Cool::ImGuiExtras::dropdown<Version>(
        //     "Version",
        //     _version_to_use_for_new_project ? _version_to_use_for_new_project->get_name().c_str() : "",
        //     _version_manager.get_all_version(),
        //     [&](Version const& version) { return _version_to_use_for_new_project && _version_to_use_for_new_project->get_name() == version.get_name(); },
        //     [&](Version const& version) { return version.get_name().c_str(); },
        //     [&](Version const& version) { _version_to_use_for_new_project = &version; }
        // );
        ImGui::End();
    }

    {
        ImGui::Begin("Projects");
        _project_manager.imgui([&](Project const& project) { launch(project); });
        ImGui::End();
    }

    // ImGui::ShowDemoWindow();
}

void App::launch(Project const& project)
{
    if (project.version() < VersionName{"Beta 19"})
    {
        ImGuiNotify::send({
            .type                 = ImGuiNotify::Type::Error,
            .title                = "Can't open project",
            .custom_imgui_content = [&]() {
                Cool::ImGuiExtras::markdown(fmt::format(
                    "Old versions cannot be used with the launcher.\n"
                    "You can download Coollab **{}** manually from [our version page](https://github.com/CoolLibs/Lab/versions)",
                    project.version().name()
                ));
            },
        });
        // TODO(Launcher) Error, old versions cannot be used with the launcher, go download them manually on github if you really want to
        return;
    }
    auto const* version = _version_manager.find(project.version());
    if (!version)
    {
        // TODO(Launcher) error
        return;
    }
    version->install_if_necessary();
    version->launch(project.file_path());
    _window.close();
}

void App::launch(std::filesystem::path const& project_file_path)
{
}