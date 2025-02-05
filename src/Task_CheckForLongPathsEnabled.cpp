#include "Task_CheckForLongPathsEnabled.hpp"
#include <ImGuiNotify/ImGuiNotify.hpp>
#include "Cool/EnableLongPaths/EnableLongPaths.hpp"
#include "Cool/Task/TaskManager.hpp"

#if defined(_WIN32)
void Task_CheckForLongPathsEnabled::execute()
{
    if (Cool::has_long_paths_enabled())
    {
        if (_notification_id.has_value())
            ImGuiNotify::close_immediately(*_notification_id);
        return;
    }

    if (!_notification_id.has_value())
    {
        _notification_id = ImGuiNotify::send({
            .type                 = ImGuiNotify::Type::Warning,
            .title                = "Need to enable long file paths support",
            .content              = "Files that are deep in too many folders and subfolders will not be able to be open. We can fix this by activating a simple option in Windows.",
            .custom_imgui_content = []() {
                if (ImGui::Button("Enable long paths"))
                    Cool::enable_long_paths_by_asking_user_permission();
            },
            .duration = std::nullopt,
        });
    }
    Cool::task_manager().submit(after(1s), std::make_shared<Task_CheckForLongPathsEnabled>(_notification_id));
}
#endif