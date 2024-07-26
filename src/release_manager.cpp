#include "release_manager.hpp"
#include <httplib.h>
#include <optional>
#include <tl/expected.hpp>
#include "fmt/format.h"
#include "release.hpp"
#include "utils.hpp"

ReleaseManager::ReleaseManager()
{
    this->latest_release_tag = get_latest_release_tag();
    this->get_all_release_available();
    // this->get_all_release_installed();
}

auto ReleaseManager::get_latest_release_tag() -> std::string
{
    std::filesystem::path url = "https://api.github.com/repos/CoolLibs/Lab/releases/latest";
    httplib::Client       cli("https://api.github.com");
    cli.set_follow_location(true);

    auto res = cli.Get(url.c_str());
    if (!res || res->status != 200)
        return fmt::format("Failed to fetch release info: {}", res ? res->status : -1);

    try
    {
        auto jsonResponse = nlohmann::json::parse(res->body);
        return jsonResponse["tag_name"];
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

auto ReleaseManager::get_all_release_available() -> std::optional<std::string>
{
    std::filesystem::path url = "https://api.github.com/repos/CoolLibs/Lab/releases";
    httplib::Client       cli("https://api.github.com");
    cli.set_follow_location(true);

    auto res = cli.Get(url.c_str());
    if (!res || res->status != 200)
        return fmt::format("Failed to fetch release info: {}", res ? res->status : -1);

    try
    {
        auto jsonResponse = nlohmann::json::parse(res->body);
        for (const auto& release : jsonResponse)
            if (!release["prerelease"]) // we keep only non pre-release version
            {
                bool latest_release = false;
                // if it's the latest release
                if (release["tag_name"] == this->latest_release_tag)
                    latest_release = true;

                for (const auto& asset : release["assets"]) // for all download file of the current release
                {
                    // check if a zip download file exists (Only one per OS)
                    if (is_zip_download(asset))
                    {
                        this->all_release_available.push_back({release["name"], asset["browser_download_url"], latest_release});
                        break;
                    }
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

auto ReleaseManager::display_all_release_available() -> void
{
    std::cout << "Releases available : " << std::endl;
    for (auto const& release : this->all_release_available)
    {
        std::cout << release.name << " : " << release.download_url;
        if (!release.is_latest)
            std::cout << " (old version)";
        else
            std::cout << " (latest)";
        if (!release.is_installed)
            std::cout << " -> ❌ not installed";
        else
            std::cout << " -> ✅ installed";
        std::cout << std::endl;
    }
}


auto ReleaseManager::install_release(bool const& latest) -> std::optional<std::string>
{
    if (latest)
    {
        for (Release& release : this->all_release_available)
            if (release.is_latest)
            {
                release.install();
                return std::nullopt;
            }
    }
    // install another release
}