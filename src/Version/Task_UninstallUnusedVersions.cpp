#include "Task_UninstallUnusedVersions.hpp"
#include "VersionManager.hpp"

void Task_UninstallUnusedVersions::execute()
{
    version_manager().uninstall_unused_versions();   
}