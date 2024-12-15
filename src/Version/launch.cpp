#include "launch.hpp"
#include "Cool/AppManager/close_application.hpp"
#include "Cool/spawn_process.hpp"
#include "installation_path.hpp"

void launch(VersionName const& version_name, std::optional<std::filesystem::path> const& project_file_path)
{
    Cool::spawn_process(
        executable_path(version_name),
        project_file_path.has_value() ? std::vector<std::string>{project_file_path->string()} : std::vector<std::string>{}
    );
    Cool::close_application_if_all_tasks_are_done(); // If some installations are still in progress we want to keep the launcher open so that they can finish. And once they are done, if we were to close the launcher it would feel weird for the user that it suddenly closed, so we just keep it open.
}