#pragma once
#include <ImGuiNotify/ImGuiNotify.hpp>
#include <map>
#include <tl/expected.hpp>
#include "Cool/Task/Task.hpp"
#include "Cool/Task/WaitToExecuteTask.hpp"
#include "ProjectToOpenOrCreate.hpp"
#include "Status.hpp"
#include "Version.hpp"
#include "VersionName.hpp"
#include "VersionRef.hpp"

class VersionManager {
public:
    VersionManager();

    void install_ifn_and_launch(VersionRef const&, ProjectToOpenOrCreate);
    void install_latest_version();

    void imgui_manage_versions();
    void imgui_versions_dropdown(VersionRef&);

    auto find(VersionName const& name) const -> Version const*;
    auto find_installed_version(VersionRef const&) const -> Version const*;
    auto latest_version() const -> Version const*;
    auto status_of_fetch_list_of_versions() const -> Status { return _status_of_fetch_list_of_versions.load(); }
    auto is_installed(VersionName const&) const -> bool;

    auto label_with_installation_icon(VersionRef const&) const -> std::string;

private:
    auto find_no_locking(VersionName const&) -> Version*;
    auto find_no_locking(VersionName const&) const -> Version const*;
    auto latest_version_no_locking() const -> Version const*;
    auto latest_installed_version_no_locking() const -> Version const*;
    auto latest_version_with_download_url_no_locking() const -> Version const*;

    void with_version_found(VersionName const& name, std::function<void(Version&)> const& callback);
    void with_version_found_or_created(VersionName const& name, std::function<void(Version&)> const& callback);
    auto has_at_least_one_version_installed() const -> bool;
    auto get_latest_installing_version_if_any() const -> std::shared_ptr<Cool::Task>;

    void install(Version const&);
    void uninstall(Version&);

    auto after_version_installed(VersionRef const& version_ref) -> std::shared_ptr<Cool::WaitToExecuteTask>;
    auto get_install_task_or_create_and_submit_it(VersionName const&) -> std::shared_ptr<Cool::Task>;

private:
    friend class Task_FetchListOfVersions;
    friend class Task_InstallVersion;

    void set_download_url(VersionName const&, std::string download_url);
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