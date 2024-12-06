#include "ReleaseManager.hpp"
#include <httplib.h>
#include <imgui.h>
#include <algorithm>
#include <optional>
#include <tl/expected.hpp>
#include "Cool/ImGui/ImGuiExtras.h"
#include "Release.hpp"
#include "download.hpp"
#include "fmt/format.h"
#include "handle_error.hpp"
#include "utils.hpp"

using namespace std::literals;

static auto fetch_all_release(std::vector<Release>& releases) -> std::optional<std::string>
{
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
                    Release _release(release["name"], asset["browser_download_url"]);
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
        for (auto const& entry : std::filesystem::directory_iterator{installation_folder()})
        {
            auto const name = entry.path().stem().string();
            if (std::none_of(releases.begin(), releases.end(), [&](Release const& release) {
                    return release.get_name() == name;
                }))
            {
                releases.emplace_back(name, ""); // TODO make download URL optional
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
    : all_release{get_all_known_releases()}
{}

auto ReleaseManager::get_all_release() const -> const std::vector<Release>&
{
    return this->all_release;
}

auto ReleaseManager::get_latest_release() const -> const Release*
{
    if (all_release.empty())
        return nullptr;
    return &this->all_release.front();
}

auto ReleaseManager::find_release(const std::string& release_version) const -> const Release*
{
    auto release_iterator = std::find_if(this->all_release.begin(), this->all_release.end(), [&](const Release& release) { return (release.get_name() == release_version); });
    if (release_iterator != this->all_release.end())
        return &*release_iterator;
    return nullptr;
}
auto ReleaseManager::display_all_release() -> void
{
    for (Release const& release : this->all_release)
    {
        std::cout << release.get_name();
        // if (release == this->get_latest_release())
        //     std::cout << " (📍 latest)";
        if (release.is_installed())
            std::cout << " -> ✅ installed";
        else
            std::cout << " -> ❌ not installed";
        std::cout << std::endl;
    }
}

// return true -> if no release have been installed
auto ReleaseManager::no_release_installed() -> bool
{
    return std::none_of(this->all_release.begin(), this->all_release.end(), [](const Release& release) { return release.is_installed(); });
}

void ReleaseManager::imgui()
{
    for (auto const& release : all_release)
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