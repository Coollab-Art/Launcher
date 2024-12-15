#include "App.hpp"
#include "Cool/ImGui/markdown.h"
#include "Cool/Task/TaskManager.hpp"
#include "ImGuiNotify/ImGuiNotify.hpp"
#include "Task_CheckForLongPathsEnabled.hpp"
#include "Version/VersionManager.hpp"
#include "Version/VersionName.hpp"
#include "Version/VersionRef.hpp"

App::App(Cool::WindowManager& windows, Cool::ViewsManager& /* views */)
    : _window{windows.main_window()}
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
        version_manager().imgui_manage_versions();
        ImGui::End();
    }

    { // New Project
        ImGui::Begin("New Project");
        if (ImGui::Button("New Project")) // TODO(Launcher) Nice big blue button
            version_manager().install_ifn_and_launch(_version_to_use_for_new_project);
        ImGui::SameLine();
        version_manager().imgui_versions_dropdown(_version_to_use_for_new_project);
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
    if (project.version_name() < VersionName{"Beta 19"})
    {
        ImGuiNotify::send({
            .type                 = ImGuiNotify::Type::Error,
            .title                = "Can't open project",
            .custom_imgui_content = [&]() {
                Cool::ImGuiExtras::markdown(fmt::format(
                    "Old versions cannot be used with the launcher.\n"
                    "You can download Coollab **{}** manually from [our release page](https://github.com/CoolLibs/Lab/releases)",
                    project.version_name().as_string()
                ));
            },
        });
        return;
    }
    version_manager().install_ifn_and_launch(project.version_name(), project.file_path());
}

void App::launch(std::filesystem::path const& project_file_path)
{
    // TODO(Launcher) open the file, find the version name, and then call version_manager.launch
}