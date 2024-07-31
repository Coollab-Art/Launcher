#include "ReleaseManager.hpp"
#include <httplib.h>
#include <algorithm>
#include <cstdlib>
#include <tl/expected.hpp>
#include "download.hpp"
#include "extractor.hpp"
#include "fmt/format.h"
#include "utils.hpp"

auto fetch_all_release() -> tl::expected<std::vector<Release>, std::string>
{
    std::filesystem::path url = "https://api.github.com/repos/CoolLibs/Lab/releases";
    httplib::Client       cli("https://api.github.com");
    cli.set_follow_location(true);

    auto res = cli.Get(url.c_str());
    if (!res || res->status != 200)
        return tl::make_unexpected(fmt::format("Failed to fetch release info: {}", res ? res->status : -1));

    try
    {
        auto                 jsonResponse = nlohmann::json::parse(res->body);
        std::vector<Release> all_release;
        for (const auto& release : jsonResponse)
            if (!release["prerelease"])                     // we keep only non pre-release version
                for (const auto& asset : release["assets"]) // for all download file of the current release
                {
                    // Good release? => zip download file exists on the release (Only one per OS)
                    if (is_zip_download(asset["browser_download_url"]))
                    {
                        Release _release(release["name"], asset["browser_download_url"]);
                        all_release.push_back(_release);
                        break;
                    }
                }
        return all_release;
    }
    catch (nlohmann::json::parse_error const& e)
    {
        return tl::make_unexpected(fmt::format("JSON parse error: {}", e.what()));
    }
    catch (std::exception& e)
    {
        return tl::make_unexpected(fmt::format("Error: {}", e.what()));
    }
}

ReleaseManager::ReleaseManager()
    : all_release{fetch_all_release()}
{}

auto ReleaseManager::get_all_release() const -> const std::vector<Release>&
{
    if (this->all_release.has_value())
        return this->all_release.value();
    throw std::runtime_error("No release found.");
}

auto ReleaseManager::get_latest_release() const -> const Release&
{
    return this->all_release->front();
}

auto ReleaseManager::display_all_release() -> void
{
    for (Release const& release : this->all_release.value())
    {
        std::cout << release.get_name();
        if (release == this->get_latest_release())
            std::cout << " (📍 latest)";
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
    return std::none_of(this->all_release.value().begin(), this->all_release.value().end(), [](const Release& release) { return release.is_installed(); });
}

auto ReleaseManager::install_release(const Release& release) -> void
{
    auto const zip = download_zip(release);
    extract_zip(*zip, release.get_name());
}

auto ReleaseManager::launch_release(const Release& release) -> void
{
    std::filesystem::path path = get_PATH() / release.get_name() / ("Coollab-" + get_OS()) / "Coollab";
    std::system(path.c_str());
}