#include "VersionManager.hpp"
#include <imgui.h>
#include <ImGuiNotify/ImGuiNotify.hpp>
#include <algorithm>
#include <open/open.hpp>
#include <optional>
#include <tl/expected.hpp>
#include <utility>
#include "Cool/ImGui/IcoMoonCodepoints.h"
#include "Cool/ImGui/ImGuiExtras.h"
#include "Cool/ImGui/ImGuiExtras_dropdown.hpp"
#include "Cool/Log/Log.hpp"
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
#include "Version/installation_path.hpp"
#include "VersionName.hpp"
#include "VersionRef.hpp"
#include "fmt/format.h"
#include "installation_path.hpp"

static auto get_all_locally_installed_versions() -> std::vector<Version>
{
    auto versions = std::vector<Version>{};
    try
    {
        for (auto const& entry : std::filesystem::directory_iterator{Path::installed_versions_folder()})
        {
            try
            {
                if (!entry.is_directory())
                    continue;
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
            catch (std::exception const& e)
            {
                Cool::Log::internal_error("Get all locally installed versions", e.what());
            }
        }
    }
    catch (std::exception const& e)
    {
        Cool::Log::internal_error("Get all locally installed versions", e.what());
    }
    std::sort(versions.begin(), versions.end());
    return versions;
}

// TODO(Launcher) Make VersionManager thread safe

VersionManager::VersionManager()
    : _versions{get_all_locally_installed_versions()}
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
            auto const* const latest_version = latest_version_with_download_url_no_locking(true /*filter_experimental_versions*/);
            if (!latest_version)
            {
                // TODO(Launcher) error, should not happen
            }
            auto const install_task = get_install_task_or_create_and_submit_it(latest_version->name);
            return after(install_task);
        }
        else if (has_at_least_one_version_installed(true /*filter_experimental_versions*/))
        {
            // We don't want to wait, use whatever version is available
            return after_nothing();
        }
        else
        {
            auto const task_install_latest_version = std::make_shared<Task_InstallVersion>(); // TODO(Launcher) When this task starts executing, it should register itself as an installing task to the version manager. Because since we don't yet know which version it will install we can't put it in the _install_tasks list immediately
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
                if (has_at_least_one_version_installed(true /*filter_experimental_versions*/))
                    return after_nothing();

                auto const install_task = get_latest_installing_version_if_any();
                if (install_task)
                    return after(install_task);
                else // NOLINT(*else-after-return)
                    return after_latest_version_installed();
            },
            [&](VersionName const& version_name) -> std::shared_ptr<Cool::WaitToExecuteTask> {
                if (is_installed(version_name, false /*filter_experimental_versions*/))
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

void VersionManager::install_latest_version(bool filter_experimental_versions)
{
    auto const* const latest_version = latest_version_no_locking(filter_experimental_versions);
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
    get_install_task_or_create_and_submit_it(version.name);
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

auto VersionManager::find(VersionName const& name, bool filter_experimental_versions) const -> Version const*
{
    // auto lock = std::unique_lock{_mutex};
    return find_no_locking(name, filter_experimental_versions);
}

auto VersionManager::find_no_locking(VersionName const& name, bool filter_experimental_versions) -> Version*
{
    auto filtered_versions = versions(filter_experimental_versions);

    auto const it = std::find_if(filtered_versions.begin(), filtered_versions.end(), [&](Version const& version) {
        return version.name == name;
    });
    if (it == filtered_versions.end())
        return nullptr;
    return &*it;
}

auto VersionManager::find_no_locking(VersionName const& name, bool filter_experimental_versions) const -> Version const*
{
    auto filtered_versions = versions(filter_experimental_versions);

    auto const it = std::find_if(filtered_versions.begin(), filtered_versions.end(), [&](Version const& version) {
        return version.name == name;
    });
    if (it == filtered_versions.end())
        return nullptr;
    return &*it;
}

auto VersionManager::find_installed_version(VersionRef const& version_ref, bool filter_experimental_versions) const -> Version const*
{
    return std::visit(
        Cool::overloaded{
            [&](LatestVersion) {
                return latest_installed_version_no_locking(filter_experimental_versions);
            },
            [&](LatestInstalledVersion) {
                return latest_installed_version_no_locking(filter_experimental_versions);
            },
            [&](VersionName const& name) {
                return find(name, filter_experimental_versions);
            }
        },
        version_ref
    );
}

void VersionManager::with_version_found(VersionName const& name, bool filter_experimental_versions, std::function<void(Version&)> const& callback)
{
    // auto lock = std::unique_lock{_mutex};

    auto* const version = find_no_locking(name, filter_experimental_versions);
    if (version == nullptr)
    {
        assert(false);
        return;
    }

    callback(*version);
}

void VersionManager::with_version_found_or_created(VersionName const& name, bool filter_experimental_versions, std::function<void(Version&)> const& callback)
{
    // auto lock = std::unique_lock{_mutex};

    auto* version = find_no_locking(name, filter_experimental_versions);
    if (version == nullptr)
    {
        auto const new_version = Version{name, InstallationStatus::NotInstalled};
        // Make sure to keep the vector sorted:
        version = &*_versions.insert(std::lower_bound(_versions.begin(), _versions.end(), new_version), new_version);
    }

    callback(*version);
}

auto VersionManager::has_at_least_one_version_installed(bool filter_experimental_versions) const -> bool
{
    // auto lock = std::shared_lock{_mutex};

    auto filtered_versions = versions(filter_experimental_versions);
    return std::any_of(filtered_versions.begin(), filtered_versions.end(), [&](Version const& version) {
        return version.installation_status == InstallationStatus::Installed;
    });
}

void VersionManager::set_download_url(VersionName const& name, std::string download_url)
{
    with_version_found_or_created(name, false /*filter_experimental_versions*/, [&](Version& version) {
        assert(!version.download_url.has_value());
        version.download_url = std::move(download_url);
    });
}

void VersionManager::set_changelog_url(VersionName const& name, std::string changelog_url)
{
    with_version_found_or_created(name, false /*  filter_experimental_versions */, [&](Version& version) {
        assert(!version.changelog_url.has_value());
        version.changelog_url = std::move(changelog_url);
    });
}

void VersionManager::set_installation_status(VersionName const& name, InstallationStatus installation_status)
{
    with_version_found_or_created(name, false /* filter_experimental_versions */, [&](Version& version) {
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
        install_latest_version(true /*filter_experimental_versions*/);
}

auto VersionManager::is_installed(VersionName const& version_name, bool filter_experimental_versions) const -> bool
{
    auto const* const version = find(version_name, filter_experimental_versions);
    if (!version)
        return false;
    return version->installation_status == InstallationStatus::Installed;
}

auto VersionManager::latest_version(bool filter_experimental_versions) const -> Version const*
{
    // auto lock = std::unique_lock{_mutex};
    return latest_version_no_locking(filter_experimental_versions);
}

auto VersionManager::latest_version_no_locking(bool filter_experimental_versions) const -> Version const*
{
    auto filtered_versions = versions(filter_experimental_versions);
    if (filtered_versions.empty())
        return nullptr;
    return &filtered_versions.front();
}

auto VersionManager::latest_installed_version_no_locking(bool filter_experimental_versions) const -> Version const*
{
    // Versions are sorted from latest to oldest so the first one we find will be the latest
    auto filtered_versions = versions(filter_experimental_versions);

    auto const it = std::find_if(filtered_versions.begin(), filtered_versions.end(), [](Version const& version) {
        return version.installation_status == InstallationStatus::Installed;
    });
    if (it == filtered_versions.end())
        return nullptr;
    return &*it;
}

auto VersionManager::latest_version_with_download_url_no_locking(bool filter_experimental_versions) const -> Version const*
{
    // Versions are sorted from latest to oldest so the first one we find will be the latest
    auto filtered_versions = versions(filter_experimental_versions);

    auto const it = std::find_if(filtered_versions.begin(), filtered_versions.end(), [](Version const& version) {
        return version.download_url.has_value();
    });
    if (it == filtered_versions.end())
        return nullptr;
    return &*it;
}

void VersionManager::imgui_manage_versions()
{
    // auto lock = std::unique_lock{_mutex};

    auto filtered_versions = versions(true /*filter_experimental_versions*/);
    for (auto& version : filtered_versions)
    {
        ImGui::PushID(&version);
        ImGui::BeginGroup();
        ImGui::SeparatorText(version.name.as_string().c_str());
        if (version.changelog_url.has_value())
        {
            if (Cool::ImGuiExtras::button_with_text_icon(ICOMOON_INFO))
                Cool::open_link(version.changelog_url->c_str());
            ImGui::SetItemTooltip("%s", fmt::format("View the changes added in {}", version.name.as_string()).c_str());
            ImGui::SameLine();
        }
        Cool::ImGuiExtras::disabled_if(version.installation_status != InstallationStatus::NotInstalled, version.installation_status == InstallationStatus::Installing ? "Installing" : "Already installed", [&]() {
            if (ImGui::Button("Install"))
                install(version);
        });
        ImGui::SameLine();
        Cool::ImGuiExtras::disabled_if(version.installation_status != InstallationStatus::Installed, version.installation_status == InstallationStatus::Installing ? "Installing" : "Not installed yet", [&]() {
            if (ImGui::Button("Uninstall"))
                uninstall(version);
        });
        ImGui::EndGroup();
        if (ImGui::BeginPopupContextItem("##version_context_menu"))
        {
            Cool::ImGuiExtras::disabled_if(version.installation_status != InstallationStatus::Installed, "Version is not installed", [&]() {
                if (ImGui::Selectable("Reveal in File Explorer"))
                    Cool::open_focused_in_explorer(executable_path(version.name));
            });
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }
}

auto VersionManager::label(VersionRef const& ref, bool filter_experimental_versions) const -> std::string
{
    return std::visit(
        Cool::overloaded{
            [&](LatestInstalledVersion) {
                auto const* version = latest_installed_version_no_locking(filter_experimental_versions);
                if (!version)
                    version = latest_version_no_locking(filter_experimental_versions);
                return fmt::format("Latest Installed ({})", version ? version->name.as_string() : "None");
            },
            [&](LatestVersion) {
                auto const* const version = latest_version_no_locking(filter_experimental_versions);
                return fmt::format("Latest ({})", version ? version->name.as_string() : "None");
            },
            [](VersionName const& name) {
                return name.as_string();
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

        auto get_label() const -> std::string
        {
            return version_manager().label(_value, true /*filter_experimental_versions*/);
        }

        void apply_value()
        {
            *_ref = _value;
        }

    private:
        VersionRef  _value;
        VersionRef* _ref;
    };

    auto entries = std::vector<DropdownEntry_VersionRef>{
        DropdownEntry_VersionRef{LatestInstalledVersion{}, &ref},
        DropdownEntry_VersionRef{LatestVersion{}, &ref},
    };
    auto filtered_versions = versions(true /*filter_experimental_versions*/);
    for (auto const& version : filtered_versions)
        entries.emplace_back(version.name, &ref);
    Cool::ImGuiExtras::dropdown("Version", label(ref, true /*filter_experimental_versions*/).c_str(), entries);
}