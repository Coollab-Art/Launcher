#pragma once

struct LauncherSettings {
    bool automatically_install_latest_version{true};

    void imgui();

private:
    // Serialization
    friend class ser20::access;
    template<class Archive>
    void serialize(Archive& archive)
    {
        archive(
            ser20::make_nvp("Automatically install latest version", automatically_install_latest_version)
        );
    }
};

inline auto launcher_settings() -> LauncherSettings&
{
    static auto instance = LauncherSettings{};
    return instance;
}