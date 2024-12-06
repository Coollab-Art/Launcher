#include "ProjectManager.hpp"
#include <imgui.h>

void ProjectManager::imgui()
{
    for (auto const& project : _projects)
    {
        if (ImGui::Selectable(project.file_path().string().c_str()))
        {
        }
    }
}

void ProjectManager::launch(std::filesystem::path const& project_path)
{
    // auto&       project = add_project_to_list(project_path);
    // auto const* release = release_manager.get(project.coollab_version());
    // if (!release)
    // {
    //     boxer::show(
    //         fmt::format("Please connect to the Internet so that we can install Coollab {}.\n This version is required to launch project \"\"", project.coollab_version().name(), project.name()),
    //         "You are offline"
    //     );
    //     return;
    // }
    // release.launch(project.file_path());
}
