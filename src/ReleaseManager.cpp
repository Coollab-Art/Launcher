#include "ReleaseManager.hpp"
#include <httplib.h>
#include <algorithm>
#include <tl/expected.hpp>
#include "fmt/format.h"
#include "release.hpp"
#include "utils.hpp"

auto get_all_release() -> tl::expected<std::vector<Release>, std::string>
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
            if (!release["prerelease"]) // we keep only non pre-release version
            {
                Release _release;
                _release.name = release["name"];

                for (const auto& asset : release["assets"]) // for all download file of the current release
                {
                    // Good release? => zip download file exists on the release (Only one per OS)
                    if (is_zip_download(asset))
                    {
                        _release.download_url = asset["browser_download_url"];
                        all_release.push_back(_release);
                        break;
                    }
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
    : all_release{get_all_release()}
{}

auto ReleaseManager::get_latest_release() const -> Release
{
    return this->all_release->front();
}

auto ReleaseManager::display_all_release() -> void
{
    for (Release const& release : this->all_release.value())
    {
        std::cout << release.name << " : " << release.download_url;
        if (release == this->get_latest_release())
            std::cout << " (📍 latest)";
        if (release.is_installed())
            std::cout << " -> ✅ installed";
        else
            std::cout << " -> ❌ not installed";
        std::cout << std::endl;
    }
}

auto ReleaseManager::latest_release_is_installed() -> bool
{
    return std::any_of(this->all_release.value().begin(), this->all_release.value().end(), [](const Release& release) { return false; });
}
