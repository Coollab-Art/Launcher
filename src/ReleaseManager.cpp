#include "ReleaseManager.hpp"
#include <httplib.h>
#include <algorithm>
#include <cstdlib>
#include <optional>
#include <tl/expected.hpp>
#include "Release.hpp"
#include "download.hpp"
#include "extractor.hpp"
#include "fmt/format.h"
#include "utils.hpp"

static auto fetch_all_release(std::vector<Release>& releases) -> std::optional<std::string>
{
    std::filesystem::path url = "https://api.github.com/repos/CoolLibs/Lab/releases";
    httplib::Client       cli("https://api.github.com");
    cli.set_follow_location(true);

    auto res = cli.Get(url.string().c_str());
    if (!res || res->status != 200)
        return fmt::format("Failed to fetch release info: {}", res ? res->status : -1);

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
        return fmt::format("JSON parse error: {}", e.what());
    }
    catch (std::exception& e)
    {
        return fmt::format("Error: {}", e.what());
    }
}

static auto get_all_locally_installed_releases(std::vector<Release>& releases) -> std::optional<std::string>
{
    try
    {
        for (auto const& entry : std::filesystem::directory_iterator{get_PATH()})
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
    catch (std::filesystem::filesystem_error const& e)
    {
        return fmt::format("Filesystem error: {}", e.what());
    }
    catch (const std::exception& e)
    {
        return fmt::format("Error: {}", e.what());
    }
    return std::nullopt;
}

static auto get_all_known_releases() -> std::vector<Release>
{
    auto res = std::vector<Release>{};
    fetch_all_release(res);                  // TODO handle error
    get_all_locally_installed_releases(res); // Must be done after fetching remote releases, because this function will not add a version if it has already been added by the previous function // TODO handle error
    std::sort(res.begin(), res.end());

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
    return &this->all_release.back();
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

auto ReleaseManager::install_release(const Release& release) -> void
{
    auto const zip = download_zip(release);
    extract_zip(*zip, release.installation_path());
    // make_file_executable();
#if defined __linux__
    std::filesystem::path path = release.executable_path();
    std::system(fmt::format("chmod u+x \"{}\"", path.string()).c_str());
#endif
}

auto ReleaseManager::launch_release(const Release& release) -> void
{
    std::filesystem::path path = release.executable_path();
    std::system(fmt::format("\"{}\"", path.string()).c_str());
}