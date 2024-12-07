#include "App.hpp"
#include "Cool/ImGui/ImGuiExtras_dropdown.hpp"
#include "Cool/ImGui/markdown.h"
#include "CoollabVersion.hpp"
#include "ImGuiNotify/ImGuiNotify.hpp"

App::App(Cool::WindowManager& windows, Cool::ViewsManager& /* views */)
    : _window{windows.main_window()}
    , _release_to_use_for_new_project{_release_manager.latest()}
{}

void App::imgui_windows()
{
    { // Versions
        ImGui::Begin("Versions");
        _release_manager.imgui();
        ImGui::End();
    }

    { // New Project
        ImGui::Begin("New Project");
        if (ImGui::Button("New Project")) // TODO(Launcher) Nice big blue button // TODO(Launcher) Should always be on top, even when we scroll through the projects
        {
            _release_to_use_for_new_project->launch(); // TODO(Launcher) Handle when no version
            _window.close();
        }
        ImGui::SameLine();
        Cool::ImGuiExtras::dropdown<Release>(
            "Version",
            _release_to_use_for_new_project ? _release_to_use_for_new_project->get_name().c_str() : "",
            _release_manager.get_all_release(),
            [&](Release const& release) { return _release_to_use_for_new_project && _release_to_use_for_new_project->get_name() == release.get_name(); },
            [&](Release const& release) { return release.get_name().c_str(); },
            [&](Release const& release) { _release_to_use_for_new_project = &release; }
        );
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
    if (project.version() < CoollabVersion{"Beta 19"})
    {
        ImGuiNotify::send({
            .type                 = ImGuiNotify::Type::Error,
            .title                = "Can't open project",
            .custom_imgui_content = [&]() {
                Cool::ImGuiExtras::markdown(fmt::format(
                    "Old versions cannot be used with the launcher.\n"
                    "You can download Coollab **{}** manually from [our release page](https://github.com/CoolLibs/Lab/releases)",
                    project.version().name()
                ));
            },
        });
        // TODO(Launcher) Error, old versions cannot be used with the launcher, go download them manually on github if you really want to
        return;
    }
    auto const* release = _release_manager.find(project.version());
    if (!release)
    {
        // TODO(Launcher) error
        return;
    }
    release->install_if_necessary();
    release->launch(project.file_path());
    _window.close();
}

void App::launch(std::filesystem::path const& project_file_path)
{
}