#include "VersionManager.hpp"
#include <imgui.h>
#include <ImGuiNotify/ImGuiNotify.hpp>
#include <algorithm>
#include <optional>
#include <tl/expected.hpp>
#include <utility>
#include "Cool/ImGui/IcoMoonCodepoints.h"
#include "Cool/ImGui/ImGuiExtras.h"
#include "Cool/ImGui/ImGuiExtras_dropdown.hpp"
#include "Cool/Task/TaskManager.hpp"
#include "Cool/Task/WaitToExecuteTask.hpp"
#include "Cool/Utils/overloaded.hpp"
#include "LauncherSettings.hpp"
#include "Path.hpp"
#include "Status.hpp"
#include "Task_FetchListOfVersions.hpp"
#include "Task_InstallVersion.hpp"
#include "Task_LaunchVersion.hpp"
#include "Version.hpp"
#include "VersionName.hpp"
#include "VersionRef.hpp"
#include "fmt/format.h"
#include "handle_error.hpp"
#include "installation_path.hpp"

static auto get_all_locally_installed_versions(std::vector<Version>& versions) -> std::optional<std::string>
{
    try
    {
        for (auto const& entry : std::filesystem::directory_iterator{Path::installed_versions_folder()})
        {
            if (!entry.is_directory())
            {
                assert(false);
                continue;
            }
            auto const name = entry.path().filename().string(); // Use filename() and not stem(), because stem() would stop at the first dot (e.g. "folder/19.0.3" would become "19" instead of "19.0.3")
            if (std::none_of(versions.begin(), versions.end(), [&](Version const& version) {
                    return version.name.as_string() == name;
                }))
            {
                auto const version_name = VersionName::from(name);
                if (version_name.has_value())
                    versions.emplace_back(Version{*version_name, InstallationStatus::Installed});
            }
        }
    }
    catch (std::exception const& e)
    {
        return fmt::format("{}", e.what());
    }
    return std::nullopt;
}

static auto get_all_known_versions() -> std::vector<Version>
{
    auto res = std::vector<Version>{};
    handle_error(get_all_locally_installed_versions(res));
    std::sort(res.begin(), res.end());
    return res;
}

// TODO(Launcher) Make VersionManager thread safe

// TODO(Launcher) When saving an image, and a project with it, this project shouldn't be registered in the launcher

VersionManager::VersionManager()
    : _versions{get_all_known_versions()}
{
    // TODO(Launcher) make sure to not send a request if we know which project to launch, and we already have that version, to save on the number of requests allowed by Github
    Cool::task_manager().submit(std::make_shared<Task_FetchListOfVersions>());
}

class WaitToExecuteTask_HasFetchedListOfVersions : public Cool::WaitToExecuteTask {
public:
    auto wants_to_execute() -> bool override { return version_manager().status_of_fetch_list_of_versions() == Status::Completed; }
    auto wants_to_cancel() -> bool override { return version_manager().status_of_fetch_list_of_versions() == Status::Canceled; }
};

static auto after_has_fetched_list_of_versions() -> std::shared_ptr<Cool::WaitToExecuteTask>
{
    return std::make_shared<WaitToExecuteTask_HasFetchedListOfVersions>();
}

auto VersionManager::after_version_installed(VersionRef const& version_ref) -> std::shared_ptr<Cool::WaitToExecuteTask>
{
    auto const after_latest_version_installed = [&]() {
        if (_status_of_fetch_list_of_versions.load() == Status::Completed)
        {
            auto const* const latest_version = latest_version_with_download_url_no_locking();
            if (!latest_version)
            {
                // TODO(Launcher) error, should not happen
            }
            auto const install_task = get_install_task_or_create_and_submit_it(latest_version->name);
            return after(install_task);
        }
        else if (has_at_least_one_version_installed())
        {
            // We don't want to wait, use whatever version is available
            return after_nothing();
        }
        else
        {
            auto const task_install_latest_version = std::make_shared<Task_InstallVersion>(); // TODO(Launcher) When this task starts executing, it should register itself as an installing task to the version manager
            Cool::task_manager().submit(after_has_fetched_list_of_versions(), task_install_latest_version);
            return after(task_install_latest_version);
        }
    };
    // TODO(Launcher) lock
    return std::visit(
        Cool::overloaded{
            [&](LatestVersion) -> std::shared_ptr<Cool::WaitToExecuteTask> {
                return after_latest_version_installed();
            },
            [&](LatestInstalledVersion) -> std::shared_ptr<Cool::WaitToExecuteTask> {
                if (has_at_least_one_version_installed())
                    return after_nothing();

                auto const install_task = get_latest_installing_version_if_any();
                if (!install_task)
                    return after_latest_version_installed();
                else
                    return after(install_task);
            },
            [&](VersionName const& version_name) -> std::shared_ptr<Cool::WaitToExecuteTask> {
                if (is_installed(version_name))
                    return after_nothing();
                auto const install_task = get_install_task_or_create_and_submit_it(version_name);
                return after(install_task);
            }
        },
        version_ref
    );
}

auto VersionManager::get_install_task_or_create_and_submit_it(VersionName const& version_name) -> std::shared_ptr<Cool::Task>
{
    auto const it = _install_tasks.find(version_name);
    if (it != _install_tasks.end())
        return it->second;

    auto const install_task = std::make_shared<Task_InstallVersion>(version_name);
    Cool::task_manager().submit(after_has_fetched_list_of_versions(), install_task);
    _install_tasks.insert(std::make_pair(version_name, install_task));
    return install_task;
}

auto VersionManager::get_latest_installing_version_if_any() const -> std::shared_ptr<Cool::Task>
{
    auto res      = std::shared_ptr<Cool::Task>{};
    auto ver_name = std::optional<VersionName>{};
    for (auto const& [version_name, task] : _install_tasks)
    {
        if (task->has_been_canceled() || task->has_been_executed())
            continue;
        if (!ver_name || *ver_name < version_name)
        {
            ver_name = version_name;
            res      = task;
        }
    }
    return res;
}

void VersionManager::install_ifn_and_launch(VersionRef const& version_ref, ProjectToOpenOrCreate project_to_open_or_create)
{
    Cool::task_manager().submit(
        after_version_installed(version_ref),
        std::make_shared<Task_LaunchVersion>(version_ref, std::move(project_to_open_or_create))
    );
}

void VersionManager::install_latest_version()
{
    auto const* const latest_version = latest_version_no_locking();
    if (latest_version && latest_version->installation_status == InstallationStatus::NotInstalled)
        install(*latest_version);
}

void VersionManager::install(Version const& version)
{
    if (version.installation_status != InstallationStatus::NotInstalled)
    {
        assert(false);
        return;
    }
    Cool::task_manager().submit(after_has_fetched_list_of_versions(), std::make_shared<Task_InstallVersion>(version.name));
}

void VersionManager::uninstall(Version& version)
{
    if (version.installation_status != InstallationStatus::Installed)
    {
        assert(false);
        return;
    }
    Cool::File::remove_folder(installation_path(version.name));
    version.installation_status = InstallationStatus::NotInstalled;
}

auto VersionManager::find(VersionName const& name) const -> Version const*
{
    // auto lock = std::unique_lock{_mutex};
    return find_no_locking(name);
}

auto VersionManager::find_no_locking(VersionName const& name) -> Version*
{
    auto const it = std::find_if(_versions.begin(), _versions.end(), [&](Version const& version) {
        return version.name == name;
    });
    if (it == _versions.end())
        return nullptr;
    return &*it;
}

auto VersionManager::find_no_locking(VersionName const& name) const -> Version const*
{
    auto const it = std::find_if(_versions.begin(), _versions.end(), [&](Version const& version) {
        return version.name == name;
    });
    if (it == _versions.end())
        return nullptr;
    return &*it;
}

auto VersionManager::find_installed_version(VersionRef const& version_ref) const -> Version const*
{
    return std::visit(
        Cool::overloaded{
            [&](LatestVersion) {
                return latest_installed_version_no_locking();
            },
            [&](LatestInstalledVersion) {
                return latest_installed_version_no_locking();
            },
            [&](VersionName const& name) {
                return find(name);
            }
        },
        version_ref
    );
}

void VersionManager::with_version_found(VersionName const& name, std::function<void(Version&)> const& callback)
{
    // auto lock = std::unique_lock{_mutex};

    auto* const version = find_no_locking(name);
    if (version == nullptr)
    {
        assert(false);
        return;
    }

    callback(*version);
}

void VersionManager::with_version_found_or_created(VersionName const& name, std::function<void(Version&)> const& callback)
{
    // auto lock = std::unique_lock{_mutex};

    auto* version = find_no_locking(name);
    if (version == nullptr)
    {
        auto const new_version = Version{name, InstallationStatus::NotInstalled};
        // Make sure to keep the vector sorted:
        version = &*_versions.insert(std::lower_bound(_versions.begin(), _versions.end(), new_version), new_version);
    }

    callback(*version);
}

auto VersionManager::has_at_least_one_version_installed() const -> bool
{
    // auto lock = std::shared_lock{_mutex};

    return std::any_of(_versions.begin(), _versions.end(), [&](Version const& version) {
        return version.installation_status == InstallationStatus::Installed;
    });
}

void VersionManager::set_download_url(VersionName const& name, std::string download_url)
{
    with_version_found_or_created(name, [&](Version& version) {
        assert(!version.download_url.has_value());
        version.download_url = download_url;
    });
}

void VersionManager::set_installation_status(VersionName const& name, InstallationStatus installation_status)
{
    with_version_found_or_created(name, [&](Version& version) {
        version.installation_status = installation_status;
    });
    if (installation_status == InstallationStatus::Installed || installation_status == InstallationStatus::NotInstalled)
    {
        auto const it = _install_tasks.find(name);
        if (it != _install_tasks.end())
            _install_tasks.erase(it);
    }
}

void VersionManager::on_finished_fetching_list_of_versions()
{
    _status_of_fetch_list_of_versions.store(Status::Completed);

    if (launcher_settings().automatically_install_latest_version)
        install_latest_version();
}

auto VersionManager::is_installed(VersionName const& version_name) const -> bool
{
    auto const* const version = find(version_name);
    if (!version)
        return false;
    return version->installation_status == InstallationStatus::Installed;
}

auto VersionManager::latest_version() const -> Version const*
{
    // auto lock = std::unique_lock{_mutex};
    return latest_version_no_locking();
}

auto VersionManager::latest_version_no_locking() const -> Version const*
{
    if (_versions.empty())
        return nullptr;
    return &_versions.front();
}

auto VersionManager::latest_installed_version_no_locking() const -> Version const*
{
    // Versions are sorted from latest to oldest so the first one we find will be the latest
    auto const it = std::find_if(_versions.begin(), _versions.end(), [](Version const& version) {
        return version.installation_status == InstallationStatus::Installed;
    });
    if (it == _versions.end())
        return nullptr;
    return &*it;
}

auto VersionManager::latest_version_with_download_url_no_locking() const -> Version const*
{
    // Versions are sorted from latest to oldest so the first one we find will be the latest
    auto const it = std::find_if(_versions.begin(), _versions.end(), [](Version const& version) {
        return version.download_url.has_value();
    });
    if (it == _versions.end())
        return nullptr;
    return &*it;
}

void VersionManager::imgui_manage_versions()
{
    // auto lock = std::unique_lock{_mutex};

    for (auto& version : _versions)
    {
        if (version.name.is_experimental() && !launcher_settings().show_experimental_versions)
            continue;

        ImGui::PushID(&version);
        ImGui::SeparatorText(version.name.as_string().c_str());
        Cool::ImGuiExtras::disabled_if(version.installation_status != InstallationStatus::NotInstalled, version.installation_status == InstallationStatus::Installing ? "Installing" : "Already installed", [&]() {
            if (ImGui::Button("Install"))
                install(version);
        });
        ImGui::SameLine();
        Cool::ImGuiExtras::disabled_if(version.installation_status != InstallationStatus::Installed, version.installation_status == InstallationStatus::Installing ? "Installing" : "Not installed yet", [&]() {
            if (ImGui::Button("Uninstall"))
                uninstall(version);
        });
        ImGui::PopID();
    }
}

auto VersionManager::label_with_installation_icon(VersionRef const& ref) const -> std::string
{
    return std::visit(
        Cool::overloaded{
            [&](LatestInstalledVersion) {
                auto const* version = latest_installed_version_no_locking();
                if (!version)
                    version = latest_version_no_locking();
                return fmt::format(
                    "{} Latest Installed ({})",
                    (version && version_manager().is_installed(version->name))
                        ? ICOMOON_CHECKBOX_CHECKED
                        : ICOMOON_CHECKBOX_UNCHECKED,
                    version ? version->name.as_string() : "None"
                );
            },
            [&](LatestVersion) {
                auto const* const version = latest_version_no_locking();
                return fmt::format(
                    "{} Latest ({})",
                    (version && version_manager().is_installed(version->name))
                        ? ICOMOON_CHECKBOX_CHECKED
                        : ICOMOON_CHECKBOX_UNCHECKED,
                    version ? version->name.as_string() : "None"
                );
            },
            [](VersionName const& name) {
                return fmt::format(
                    "{} {}",
                    version_manager().is_installed(name)
                        ? ICOMOON_CHECKBOX_CHECKED
                        : ICOMOON_CHECKBOX_UNCHECKED,
                    name.as_string()
                );
            }
        },
        ref
    );
}

void VersionManager::imgui_versions_dropdown(VersionRef& ref)
{
    // auto lock = std::shared_lock{_mutex};

    class DropdownEntry_VersionRef {
    public:
        DropdownEntry_VersionRef(VersionRef value, VersionRef* ref)
            : _value{std::move(value)}
            , _ref{ref}
        {}

        auto is_selected() -> bool
        {
            return _value == *_ref;
        }

        auto get_label() -> const char*
        {
            _label = version_manager().label_with_installation_icon(_value);
            return _label.c_str();
        }

        void apply_value()
        {
            *_ref = _value;
        }

    private:
        VersionRef  _value;
        VersionRef* _ref;
        std::string _label;
    };

    auto entries = std::vector<DropdownEntry_VersionRef>{
        DropdownEntry_VersionRef{LatestInstalledVersion{}, &ref},
        DropdownEntry_VersionRef{LatestVersion{}, &ref},
    };
    for (auto const& version : _versions)
    {
        if (version.name.is_experimental() && !launcher_settings().show_experimental_versions)
            continue;
        entries.emplace_back(version.name, &ref);
    }
    Cool::ImGuiExtras::dropdown("Version", label_with_installation_icon(ref).c_str(), entries);
}