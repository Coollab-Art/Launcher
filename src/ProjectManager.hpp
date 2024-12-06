#pragma once
#include "Project.hpp"

class ProjectManager {
public:
    void imgui();
    void launch(std::filesystem::path const& project_path);

private:
    std::vector<Project> _projects{};

private:
    // Serialization
    friend class ser20::access;
    template<class Archive>
    void serialize(Archive& archive)
    {
        archive(
            ser20::make_nvp("Projects", _projects)
        );
    }
};