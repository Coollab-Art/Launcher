#include "App.hpp"
#include "Cool/ImGui/ImGuiExtras_dropdown.hpp"

App::App(Cool::WindowManager& windows, Cool::ViewsManager& /* views */)
    : _window{windows.main_window()}
    , _release_to_use_for_new_project{_release_manager.get_latest_release()}
{}

void App::imgui_windows()
{
    ImGui::Begin("Main");
    if (ImGui::Button("New Project"))
    {
        _release_to_use_for_new_project->launch();
        glfwSetWindowShouldClose(_window.glfw(), true);
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
    ImGui::ShowDemoWindow();
}