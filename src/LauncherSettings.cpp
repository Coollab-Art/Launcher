#include "LauncherSettings.hpp"
#include "Cool/ImGui/ImGuiExtras.h"

void LauncherSettings::imgui()
{
    Cool::ImGuiExtras::toggle("Automatically install latest version", &automatically_install_latest_version);
}