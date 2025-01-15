#pragma once

// TODO(Launcher) Serialize
struct LauncherSettings {
    bool automatically_install_latest_version{true};

    void imgui();
};

inline auto launcher_settings() -> LauncherSettings&
{
    static auto instance = LauncherSettings{};
    return instance;
}