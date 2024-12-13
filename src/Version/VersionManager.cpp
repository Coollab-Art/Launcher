#include "VersionManager.hpp"
#include <httplib.h>
#include <imgui.h>
#include <algorithm>
#include <optional>
#include <tl/expected.hpp>
#include "Cool/ImGui/ImGuiExtras.h"
#include "Path.hpp"
#include "Version.hpp"
#include "VersionName.hpp"
#include "fmt/format.h"
#include "handle_error.hpp"
#include "nlohmann/json.hpp"
#include "utils.hpp"

// TODO(Launcher) block install/uninstall buttons while version is installing

static auto fetch_all_version(std::vector<Version>& versions) -> std::optional<std::string>
{
    // TODO(Launcher) do this in a task, and make the task loop
    std::filesystem::path url = "https://api.github.com/repos/CoolLibs/Lab/versions";
    httplib::Client       cli("https://api.github.com");
    cli.set_follow_location(true);
    if (!versions.empty())
    {
        // If we have at least one version installed, there is no need to wait too long to check if new versions are available
        // This prevents Coollab from beeing slow to launch when you have a bad internet connection
        cli.set_read_timeout(500ms);
        cli.set_write_timeout(500ms);
        cli.set_connection_timeout(500ms);
    }

    auto res = cli.Get(url.string());
    if (!res || res->status != 200)
        return fmt::format("Failed to fetch version info: {}", httplib::to_string(res.error()));

    try
    {
        auto jsonResponse = nlohmann::json::parse(res->body);
        for (const auto& version : jsonResponse)
            // if (!version["preversion"])                     // we keep only non pre-version version // TODO actually, Experimental versions will be marked as preversion, but we still want to have them
            for (const auto& asset : version["assets"]) // for all download file of the current version
            {
                // Good version? => zip download file exists on the version (Only one per OS)
                if (is_zip_download(asset["browser_download_url"]))
                {
                    auto const version_name = VersionName{version["name"]};
                    if (!version_name.is_valid())
                        continue;
                    Version _version(version_name, asset["browser_download_url"]);
                    versions.push_back(_version);
                    break;
                }
            }
        return std::nullopt;
    }
    catch (nlohmann::json::parse_error const& e)
    {
        return fmt::format("JSON parsing error: {}", e.what());
    }
    catch (std::exception& e)
    {
        return fmt::format("{}", e.what());
    }
}

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
                    return version.get_name() == name;
                }))
            {
                versions.emplace_back(VersionName{name}, ""); // TODO make download URL optional
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
    handle_error(fetch_all_version(res));
    handle_error(get_all_locally_installed_versions(res)); // Must be done after fetching remote versions, because this function will not add a version if it has already been added by the previous function
    std::sort(res.begin(), res.end(), [](Version const& a, Version const& b) {
        return a.version() > b.version();
    });

    return res;
}

VersionManager::VersionManager()
    : _versions{get_all_known_versions()}
{}

auto VersionManager::latest() const -> Version const*
{
    if (_versions.empty())
        return nullptr;
    return &_versions.front();
}

auto VersionManager::find(VersionName const& version_name) const -> Version const*
{
    auto const it = std::find_if(_versions.begin(), _versions.end(), [&](Version const& version) {
        return version.version() == version_name;
    });
    if (it == _versions.end())
        return nullptr;
    return &*it; // TODO(Launcher) If we hand out references to our versions, we should store them in a list, not a vector, to make sure iterators don't invalidate
}

// return true -> if no version have been installed
auto VersionManager::no_version_installed() -> bool
{
    return std::none_of(_versions.begin(), _versions.end(), [](const Version& version) { return version.is_installed(); });
}

void VersionManager::imgui()
{
    for (auto const& version : _versions)
    {
        ImGui::PushID(&version);
        ImGui::SeparatorText(version.get_name().c_str());
        Cool::ImGuiExtras::disabled_if(version.is_installed(), "Version is already installed", [&]() {
            if (ImGui::Button("Install"))
                version.install();
        });
        ImGui::SameLine();
        Cool::ImGuiExtras::disabled_if(!version.is_installed(), "Version is not installed", [&]() {
            if (ImGui::Button("Uninstall"))
                version.uninstall();
        });
        ImGui::PopID();
    }
}