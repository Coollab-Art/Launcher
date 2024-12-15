#pragma once
#include <filesystem>
#include <map>
#include <shared_mutex>
#include <tl/expected.hpp>
#include "Version.hpp"
#include "VersionName.hpp"
#include "VersionRef.hpp"

class VersionManager {
public:
    VersionManager();

    void install_ifn_and_launch(VersionRef const&, std::optional<std::filesystem::path> const& project_file_path = {});

    void imgui_manage_versions();
    void imgui_versions_dropdown(VersionRef&);

private:
    auto find_no_locking(VersionName const& name) -> Version*;
    void with_version_found(VersionName const& name, std::function<void(Version&)> const& callback);
    void with_version_found_or_created(VersionName const& name, std::function<void(Version&)> const& callback);

    void install(Version const&);
    void uninstall(Version&);

public: // TODO
    auto latest_version_no_locking() -> Version*;
    auto latest_installed_version_no_locking() -> Version*;

private:
    friend class Task_FindVersionsAvailableOnline;
    friend class Task_InstallVersion;

    void set_download_url(VersionName const&, std::string download_url);
    void set_installation_status(VersionName const&, InstallationStatus);

private:
    std::vector<Version>      _versions{}; // Sorted, from latest to oldest version
    mutable std::shared_mutex _mutex{};

    std::map<VersionName, std::optional<std::filesystem::path>> _project_to_launch_after_version_installed{};
    mutable std::mutex                                          _project_to_launch_after_version_installed_mutex{};
};

inline auto version_manager() -> VersionManager&
{
    static auto instance = VersionManager{};
    return instance;
}