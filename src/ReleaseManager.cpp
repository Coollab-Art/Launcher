#include "ReleaseManager.hpp"
#include <httplib.h>
#include <imgui.h>
#include <algorithm>
#include <optional>
#include <tl/expected.hpp>
#include "Cool/File/File.h"
#include "Cool/ImGui/ImGuiExtras.h"
#include "CoollabVersion.hpp"
#include "Path.hpp"
#include "Release.hpp"
#include "fmt/format.h"
#include "handle_error.hpp"
#include "nlohmann/json.hpp"
#include "utils.hpp"

static auto fetch_all_release(std::vector<Release>& releases) -> std::optional<std::string>
{
    // TODO(Launcher) do this in a task
    std::filesystem::path url = "https://api.github.com/repos/CoolLibs/Lab/releases";
    httplib::Client       cli("https://api.github.com");
    cli.set_follow_location(true);
    if (!releases.empty())
    {
        // If we have at least one version installed, there is no need to wait too long to check if new versions are available
        // This prevents Coollab from beeing slow to launch when you have a bad internet connection
        cli.set_read_timeout(500ms);
        cli.set_write_timeout(500ms);
        cli.set_connection_timeout(500ms);
    }

    auto res = cli.Get(url.string());
    if (!res || res->status != 200)
        return fmt::format("Failed to fetch release info: {}", httplib::to_string(res.error()));

    try
    {
        auto jsonResponse = nlohmann::json::parse(res->body);
        for (const auto& release : jsonResponse)
            // if (!release["prerelease"])                     // we keep only non pre-release version // TODO actually, Experimental versions will be marked as prerelease, but we still want to have them
            for (const auto& asset : release["assets"]) // for all download file of the current release
            {
                // Good release? => zip download file exists on the release (Only one per OS)
                if (is_zip_download(asset["browser_download_url"]))
                {
                    auto const version = CoollabVersion{release["name"]};
                    if (!version.is_valid())
                        continue;
                    Release _release(version, asset["browser_download_url"]);
                    releases.push_back(_release);
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

static auto get_all_locally_installed_releases(std::vector<Release>& releases) -> std::optional<std::string>
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
            if (std::none_of(releases.begin(), releases.end(), [&](Release const& release) {
                    return release.get_name() == name;
                }))
            {
                releases.emplace_back(CoollabVersion{name}, ""); // TODO make download URL optional
            }
        }
    }
    catch (std::exception const& e)
    {
        return fmt::format("{}", e.what());
    }
    return std::nullopt;
}

static auto get_all_known_releases() -> std::vector<Release>
{
    auto res = std::vector<Release>{};
    handle_error(fetch_all_release(res));
    handle_error(get_all_locally_installed_releases(res)); // Must be done after fetching remote releases, because this function will not add a version if it has already been added by the previous function
    std::sort(res.begin(), res.end(), [](Release const& a, Release const& b) {
        return a.version() > b.version();
    });

    return res;
}

ReleaseManager::ReleaseManager()
    : _releases{get_all_known_releases()}
{}

auto ReleaseManager::get_all_release() const -> const std::vector<Release>&
{
    return _releases;
}

auto ReleaseManager::latest() const -> Release const*
{
    if (_releases.empty())
        return nullptr;
    return &_releases.front();
}

auto ReleaseManager::find(CoollabVersion const& version) const -> Release const*
{
    auto const it = std::find_if(_releases.begin(), _releases.end(), [&](Release const& release) {
        return release.version() == version;
    });
    if (it == _releases.end())
        return nullptr;
    return &*it; // TODO(Launcher) If we hand out references to our releases, we should store them in a list, not a vector, to make sure iterators don't invalidate
}

// return true -> if no release have been installed
auto ReleaseManager::no_release_installed() -> bool
{
    return std::none_of(_releases.begin(), _releases.end(), [](const Release& release) { return release.is_installed(); });
}

void ReleaseManager::imgui()
{
    for (auto const& release : _releases)
    {
        ImGui::PushID(&release);
        ImGui::SeparatorText(release.get_name().c_str());
        Cool::ImGuiExtras::disabled_if(release.is_installed(), "Version is already installed", [&]() {
            if (ImGui::Button("Install"))
                release.install();
        });
        ImGui::SameLine();
        Cool::ImGuiExtras::disabled_if(!release.is_installed(), "Version is not installed", [&]() {
            if (ImGui::Button("Uninstall"))
                release.uninstall();
        });
        ImGui::PopID();
    }
}