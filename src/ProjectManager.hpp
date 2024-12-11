#pragma once
#include "Project.hpp"

class ProjectManager {
public:
    ProjectManager();

    void imgui(std::function<void(Project const&)> const& launch_project);
    void launch(std::filesystem::path const& project_path);

    auto has_some_projects() const -> bool { return !_projects.empty(); }

private:
    std::vector<Project> _projects{};
};