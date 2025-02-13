#include "LauncherSettings.hpp"
#include "Cool/ImGui/ImGuiExtras.h"
#include "Version/VersionManager.hpp"

void LauncherSettings::imgui()
{
    bool b{false};

    if (Cool::ImGuiExtras::toggle("Automatically install latest version", &automatically_install_latest_version))
    {
        b = true;
        if (automatically_install_latest_version)
            version_manager().install_latest_version();
    }

    b |= Cool::ImGuiExtras::toggle("Show experimental versions", &show_experimental_versions);
    Cool::ImGuiExtras::help_marker("These versions are highly unstable and should only be used if you know what you are doing");

    if (b)
        _serializer.save();
}