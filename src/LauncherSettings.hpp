#pragma once
#include "Cool/Serialization/Json.hpp"
#include "Cool/Serialization/JsonAutoSerializer.hpp"

struct LauncherSettings {
    bool automatically_install_latest_version{true};
    bool automatically_upgrade_projects_to_latest_compatible_version{true};
    bool show_experimental_versions{false};

    void imgui();
    void save() { _serializer.save(); }

private:
    Cool::JsonAutoSerializer _serializer{
        "user_settings_launcher.json",
        false /*autosave_when_destroyed*/, // This is a static instance, so saving it in the destructor is dangerous because we don't know when it will happen exactly. Instead, we call save manually in App::on_shutdown()
        [&](nlohmann::json const& json) {
            Cool::json_get(json, "Automatically install latest version", automatically_install_latest_version);
            Cool::json_get(json, "Automatically upgrade projects to latest compatible version", automatically_upgrade_projects_to_latest_compatible_version);
            Cool::json_get(json, "Show experimental versions", show_experimental_versions);
        },
        [&](nlohmann::json& json) {
            Cool::json_set(json, "Automatically install latest version", automatically_install_latest_version);
            Cool::json_set(json, "Automatically upgrade projects to latest compatible version", automatically_upgrade_projects_to_latest_compatible_version);
            Cool::json_set(json, "Show experimental versions", show_experimental_versions);
        }
    };
};

inline auto launcher_settings() -> LauncherSettings&
{
    static auto instance = LauncherSettings{};
    return instance;
}