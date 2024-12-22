#include "Task_WaitForDownloadUrlToInstallVersion.hpp"
#include <ImGuiNotify/ImGuiNotify.hpp>
#include <optional>
#include "Cool/Task/TaskManager.hpp"
#include "Task_InstallVersion.hpp"
#include "VersionManager.hpp"

Task_WaitForDownloadUrlToInstallVersion::Task_WaitForDownloadUrlToInstallVersion(VersionName version_name, ImGuiNotify::NotificationId notification_id, std::shared_ptr<std::atomic<bool>> do_cancel)
    : _version_name{std::move(version_name)}
    , _notification_id{notification_id}
    , _cancel{std::move(do_cancel)}
{
    if (_notification_id == ImGuiNotify::NotificationId{})
    {
        _notification_id = ImGuiNotify::send({
            .type                 = ImGuiNotify::Type::Info,
            .title                = name(),
            .content              = "You are offline\nWaiting for an Internet connection",
            .custom_imgui_content = [cancel = _cancel]() {
                if (ImGui::Button("Cancel"))
                    cancel->store(true);
            },
            .duration    = std::nullopt,
            .is_closable = false,
        });
    }
}

void Task_WaitForDownloadUrlToInstallVersion::do_work()
{
    if (_cancel->load())
    {
        ImGuiNotify::close_immediately(_notification_id);
        return;
    }

    version_manager().with_version_found(_version_name, [&](Version const& version) {
        if (version.download_url.has_value())
            Cool::task_manager().submit(std::make_shared<Task_InstallVersion>(version.name, *version.download_url, _notification_id));
        else
            Cool::task_manager().run_small_task_in(100ms, std::make_shared<Task_WaitForDownloadUrlToInstallVersion>(_version_name, _notification_id, _cancel));
    });
}