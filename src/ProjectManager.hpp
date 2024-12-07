#pragma once
#include "Project.hpp"

class ProjectManager {
public:
    ProjectManager();

    void imgui(std::function<void(Project const&)> const& launch_project);
    void launch(std::filesystem::path const& project_path);

private:
    std::vector<Project> _projects{};
};