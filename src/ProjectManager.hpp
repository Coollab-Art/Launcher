#pragma once
#include "Project.hpp"

class ProjectManager {
public:
    ProjectManager();

    void imgui();
    void launch(std::filesystem::path const& project_path);

private:
    std::vector<Project> _projects{};
};