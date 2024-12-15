#include "VersionManager.hpp"
#include <httplib.h>
#include <imgui.h>
#include <ImGuiNotify/ImGuiNotify.hpp>
#include <algorithm>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <tl/expected.hpp>
#include <utility>
#include <wcam/src/overloaded.hpp>
#include "Cool/ImGui/ImGuiExtras.h"
#include "Cool/ImGui/ImGuiExtras_dropdown.hpp"
#include "Cool/Task/TaskManager.hpp"
#include "Path.hpp"
#include "Task_InstallVersion.hpp"
#include "Version.hpp"
#include "VersionName.hpp"
#include "VersionRef.hpp"
#include "fmt/format.h"
#include "handle_error.hpp"
#include "installation_path.hpp"
#include "launch.hpp"
#include "nlohmann/json.hpp"
#include "utils.hpp"

class Task_FindVersionsAvailableOnline : public Cool::Task {
public:
    void do_work() override
    {
        auto cli = httplib::Client{"https://api.github.com"};
        cli.set_follow_location(true);
        auto const res = cli.Get("https://api.github.com/repos/CoolLibs/Lab/releases", [&](uint64_t, uint64_t) {
            return !_cancel.load();
        });
        if (!res || res->status != 200)
        {
            ImGuiNotify::send({
                .type    = ImGuiNotify::Type::Warning,
                .title   = "Failed to check for new versions online",
                .content = fmt::format("Failed to fetch version info: {}", httplib::to_string(res.error())),
            });
            return;
        }

        try // TODO(Launcher) Put a try on each iteration of the loop, so that an exception doesn't prevent other versions from loading
        {
            auto const json_response = nlohmann::json::parse(res->body);
            for (auto const& version : json_response)
            {
                if (_cancel.load())
                    return;
                // if (!version["prerelease"])                     // we keep only non pre-release version // TODO actually, Experimental versions will be marked as preversion, but we still want to have them
                for (const auto& asset : version["assets"]) // for all download file of the current version
                {
                    if (!is_zip_download(asset["browser_download_url"])) // Good version? => zip download file exists on the version (Only one per OS)
                        continue;

                    auto const version_name = VersionName{version["name"]};
                    if (!version_name.is_valid())
                        continue;
                    version_manager().set_download_url(version_name, asset["browser_download_url"]);
                    break;
                }
            }
        }
        catch (nlohmann::json::parse_error const& e)
        {
            // return fmt::format("JSON parsing error: {}", e.what());
        }
        catch (std::exception& e)
        {
            // return fmt::format("{}", e.what());
        }
    }

    void cancel() override { _cancel.store(true); }
    auto needs_user_confirmation_to_cancel_when_closing_app() const -> bool override { return false; }

private:
    std::atomic<bool> _cancel{false};
};

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
                versions.emplace_back(VersionName{name}, InstallationStatus::Installed);
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
    Cool::task_manager().submit(std::make_shared<Task_FindVersionsAvailableOnline>());
}

void VersionManager::install_ifn_and_launch(VersionRef const& version_ref, std::optional<std::filesystem::path> const& project_file_path)
{
    auto lock = std::shared_lock{_mutex};

    auto const* const version = std::visit(
        wcam::overloaded{
            [&](LatestVersion) {
                return latest_version_no_locking();
            },
            [&](LatestInstalledVersion) {
                return latest_installed_version_no_locking();
            },
            [&](VersionName const& name) {
                return find_no_locking(name);
            }
        },
        version_ref
    );

    if (!version)
    {
        // TODO(Launcher) handle error
        return;
    }

    {
        auto lock = std::unique_lock{_project_to_launch_after_version_installed_mutex};
        if (version->installation_status == InstallationStatus::Installed)
        {
            launch(version->name, project_file_path);
        }
        else
        {
            if (version->installation_status == InstallationStatus::NotInstalled)
                install(*version);
            _project_to_launch_after_version_installed[version->name] = // Must be done after calling install() so that the Launch notification will be above the Install one
                {
                    project_file_path,
                    ImGuiNotify::send({
                        .type     = ImGuiNotify::Type::Info,
                        .title    = "Launch",
                        .content  = fmt::format("Waiting for {} to install before we can launch the project", version->name.as_string()),
                        .duration = std::nullopt,
                    })
                };
        }
    }
}

void VersionManager::install(Version const& version)
{
    if (version.installation_status != InstallationStatus::NotInstalled)
    {
        assert(false);
        return;
    }

    // if (!version.download_url)
    //     return tl::make_unexpected("Version not found"); // TODO(Launcher) wait for get_all_versions request to finish
    Cool::task_manager().submit(std::make_shared<Task_InstallVersion>(version.name, *version.download_url));
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

auto VersionManager::find_no_locking(VersionName const& name) -> Version*
{
    auto const it = std::find_if(_versions.begin(), _versions.end(), [&](Version const& version) {
        return version.name == name;
    });
    if (it == _versions.end())
        return nullptr;
    return &*it;
}

void VersionManager::with_version_found(VersionName const& name, std::function<void(Version&)> const& callback)
{
    auto lock = std::unique_lock{_mutex};

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
    auto lock = std::unique_lock{_mutex};

    auto* version = find_no_locking(name);
    if (version == nullptr)
    {
        auto const new_version = Version{name, InstallationStatus::NotInstalled};
        // Make sure to keep the vector sorted:
        version = &*_versions.insert(std::lower_bound(_versions.begin(), _versions.end(), new_version), new_version);
    }

    callback(*version);
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
    with_version_found(name, [&](Version& version) {
        version.installation_status = installation_status;
    });
    if (installation_status == InstallationStatus::Installed)
    {
        auto       lock = std::unique_lock{_project_to_launch_after_version_installed_mutex};
        auto const it   = _project_to_launch_after_version_installed.find(name);
        if (it != _project_to_launch_after_version_installed.end())
        {
            ImGuiNotify::close_immediately(it->second.notification_id);
            launch(name, it->second.path);
        }
    }
    else if (installation_status == InstallationStatus::NotInstalled)
    {
        // Installation has been canceled, so also cancel the project that was supposed to launch afterwards
        auto       lock = std::unique_lock{_project_to_launch_after_version_installed_mutex};
        auto const it   = _project_to_launch_after_version_installed.find(name);
        if (it != _project_to_launch_after_version_installed.end())
        {
            ImGuiNotify::close_immediately(it->second.notification_id);
            _project_to_launch_after_version_installed.erase(it);
        }
    }
}

auto VersionManager::latest_version_no_locking() -> Version*
{
    if (_versions.empty())
        return nullptr;
    return &_versions.front();
}

auto VersionManager::latest_installed_version_no_locking() -> Version*
{
    auto const it = std::find_if(_versions.begin(), _versions.end(), [](Version const& version) {
        return version.installation_status == InstallationStatus::Installed;
    });
    if (it == _versions.end())
        return nullptr;
    return &*it;
}

void VersionManager::imgui_manage_versions()
{
    auto lock = std::unique_lock{_mutex};

    for (auto& version : _versions)
    {
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

static auto label(VersionRef const& ref) -> std::string
{
    return std::visit(
        wcam::overloaded{
            [&](LatestVersion) {
                auto const* const version = version_manager().latest_version_no_locking();
                return fmt::format("Latest ({})", version ? version->name.as_string() : "None");
                // return _label.c_str();
            },
            [&](LatestInstalledVersion) {
                auto const* const version = version_manager().latest_installed_version_no_locking();
                return fmt::format("Latest Installed ({})", version ? version->name.as_string() : "None");
                // return _label.c_str();
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
    auto lock = std::shared_lock{_mutex};

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
            return std::visit(
                wcam::overloaded{
                    [&](LatestVersion) {
                        auto const version = version_manager().latest_version_no_locking();
                        _label             = fmt::format("Latest ({})", version ? version->name.as_string() : "None");
                        return _label.c_str();
                    },
                    [&](LatestInstalledVersion) {
                        auto const version = version_manager().latest_installed_version_no_locking();
                        _label             = fmt::format("Latest Installed ({})", version ? version->name.as_string() : "None");
                        return _label.c_str();
                    },
                    [](VersionName const& name) {
                        return name.as_string().c_str(); // TODO indicate if this is installed or not, with a small icon
                    }
                },
                _value
            );
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
        DropdownEntry_VersionRef{LatestVersion{}, &ref},
        DropdownEntry_VersionRef{LatestInstalledVersion{}, &ref},
    };
    for (auto const& version : _versions)
        entries.emplace_back(version.name, &ref);
    Cool::ImGuiExtras::dropdown("Version", label(ref).c_str(), entries);
}