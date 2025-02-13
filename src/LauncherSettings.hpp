#pragma once
#include "Cool/Serialization/Json.hpp"
#include "Cool/Serialization/JsonAutoSerializer.hpp"

struct LauncherSettings {
    bool automatically_install_latest_version{true};
    bool show_experimental_versions{false};

    void imgui();

private:
    Cool::JsonAutoSerializer _serializer{
        "user_settings_launcher.json",
        true /*autosave_when_destroyed*/, // Even if the user doesn't change the settings, we will save the settings they have seen once, so that if a new version of the software comes with new settings, we will not change settings that the user is used to
        [&](nlohmann::json const& json) {
            Cool::json_get(json, "Automatically install latest version", automatically_install_latest_version);
            Cool::json_get(json, "Show experimental versions", show_experimental_versions);
        },
        [&](nlohmann::json& json) {
            Cool::json_set(json, "Automatically install latest version", automatically_install_latest_version);
            Cool::json_set(json, "Show experimental versions", show_experimental_versions);
        }
    };
};

inline auto launcher_settings() -> LauncherSettings&
{
    static auto instance = LauncherSettings{};
    return instance;
}