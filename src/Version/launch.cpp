#include "launch.hpp"
#include "Cool/spawn_process.hpp"
#include "installation_path.hpp"

void launch(VersionName const& version_name, std::optional<std::filesystem::path> const& project_file_path)
{
    Cool::spawn_process(
        executable_path(version_name),
        project_file_path.has_value() ? std::vector<std::string>{project_file_path->string()} : std::vector<std::string>{}
    );
    // _window.close(); // TODO(Launcher) Only close once all tasks are done (minify/hide window while we wait ?)
}