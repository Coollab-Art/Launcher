#pragma once
#include <ImGuiNotify/ImGuiNotify.hpp>
#include <map>
#include <tl/expected.hpp>
#include "Cool/Task/Task.hpp"
#include "Cool/Task/WaitToExecuteTask.hpp"
#include "LauncherSettings.hpp"
#include "ProjectToOpenOrCreate.hpp"
#include "Status.hpp"
#include "Task_InstallVersion.hpp"
#include "Version.hpp"
#include "VersionName.hpp"
#include "VersionRef.hpp"
#include "range/v3/view.hpp"

class VersionManager {
public:
    VersionManager();

    void install_ifn_and_launch(VersionRef const&, ProjectToOpenOrCreate);
    void install_latest_version(bool filter_experimental_versions);

    void imgui_manage_versions();
    void imgui_versions_dropdown(VersionRef&);

    auto find(VersionName const& name, bool filter_experimental_versions) const -> Version const*;
    auto find_installed_version(VersionRef const&, bool filter_experimental_versions) const -> Version const*;
    auto latest_version(bool filter_experimental_versions) const -> Version const*;
    auto status_of_fetch_list_of_versions() const -> Status { return _status_of_fetch_list_of_versions.load(); }
    auto is_installed(VersionName const&, bool filter_experimental_versions) const -> bool;

    void uninstall_unused_versions();

    auto label(VersionRef const&, bool filter_experimental_versions) const -> std::string;

private:
    auto find_no_locking(VersionName const&, bool filter_experimental_versions) -> Version*;
    auto find_no_locking(VersionName const&, bool filter_experimental_versions) const -> Version const*;
    auto latest_version_no_locking(bool filter_experimental_versions) const -> Version const*;
    auto latest_installed_version_no_locking(bool filter_experimental_versions) const -> Version const*;
    auto latest_version_with_download_url_no_locking(bool filter_experimental_versions) const -> Version const*;

    void with_version_found(VersionName const& name, bool filter_experimental_versions, std::function<void(Version&)> const& callback);
    void with_version_found_or_created(VersionName const& name, bool filter_experimental_versions, std::function<void(Version&)> const& callback);
    auto has_at_least_one_version_installed(bool filter_experimental_versions) const -> bool;
    auto get_latest_installing_version_if_any(bool filter_experimental_versions) const -> std::shared_ptr<Cool::Task>;

    void install(Version const&);
    void uninstall(Version&);

    auto after_version_installed(VersionRef const& version_ref) -> std::shared_ptr<Cool::WaitToExecuteTask>;
    auto get_install_task_or_create_and_submit_it(VersionName const&) -> std::shared_ptr<Cool::Task>;

    auto versions(bool filter_experimental_versions) const
    {
        return _versions | ranges::views::filter([filter_experimental_versions](Version const& version) {
                   return !version.name.is_experimental() || !filter_experimental_versions || launcher_settings().show_experimental_versions;
               });
    }
    auto versions(bool filter_experimental_versions)
    {
        return _versions | ranges::views::filter([filter_experimental_versions](Version const& version) {
                   return !version.name.is_experimental() || !filter_experimental_versions || launcher_settings().show_experimental_versions;
               });
    }
    auto installed_versions(bool filter_experimental_versions) const
    {
        return versions(filter_experimental_versions) | ranges::views::filter([](Version const& version) {
                   return version.installation_status == InstallationStatus::Installed;
               });
    }
    auto installed_versions(bool filter_experimental_versions)
    {
        return versions(filter_experimental_versions) | ranges::views::filter([](Version const& version) {
                   return version.installation_status == InstallationStatus::Installed;
               });
    }

private:
    friend class Task_FetchListOfVersions;
    friend class Task_InstallVersion;

    void set_download_url(VersionName const&, std::string download_url);
    void set_changelog_url(VersionName const&, std::string changelog_url);
    void set_installation_status(VersionName const&, InstallationStatus);
    void on_finished_fetching_list_of_versions();

private:
    std::vector<Version> _versions{}; // Sorted, from latest to oldest version
    // mutable std::shared_mutex _mutex{};

    std::atomic<Status>                                _status_of_fetch_list_of_versions{Status::Waiting};
    std::map<VersionName, std::shared_ptr<Cool::Task>> _install_tasks{};
};

inline auto version_manager() -> VersionManager&
{
    static auto instance = VersionManager{};
    return instance;
}