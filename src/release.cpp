#include "release.hpp"
#include <filesystem>
#include <string>
#include <tl/expected.hpp>
#include <unordered_map>
#include "fmt/core.h"
#include "httplib.h"
#include "utils.hpp"

namespace fs = std::filesystem;

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
        std::cout << "Releases available : " << std::endl;
        for (const auto& release : jsonResponse)
            if (!release["prerelease"])
            {
                for (const auto& asset : release["assets"])
                {
                    if (!(std::string(asset["browser_download_url"]).find(get_OS() + ".zip") != std::string::npos))
                        continue;
                    std::cout << release["name"] << " -> " << asset["name"] << "\n";
                    all_release.push_back({release["name"], asset["browser_download_url"]});
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

auto get_release(std::string_view const& version) -> tl::expected<nlohmann::json, std::string>
{
    std::string url = fmt::format("https://api.github.com/repos/CoolLibs/Lab/releases/tags/{}", version);

    httplib::Client cli("https://api.github.com");
    cli.set_follow_location(true);

    auto res = cli.Get(url.c_str());

    if (!res || res->status != 200)
    {
        return tl::make_unexpected(fmt::format("Failed to fetch release info: {}", res ? res->status : -1));
    }

    try
    {
        auto jsonResponse = nlohmann::json::parse(res->body);
        if (!jsonResponse.contains("assets") || jsonResponse["assets"].empty())
        {
            return tl::make_unexpected("No assets found in the release.");
        }
        nlohmann::json const& assets = jsonResponse["assets"];
        return assets;
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

// auto get_release(std::string_view const& version) -> tl::expected<nlohmann::json, std::string>
// {
//     std::string url = fmt::format("https://api.github.com/repos/CoolLibs/Lab/releases/tags/{}", version);

//     httplib::Client cli("https://api.github.com");
//     cli.set_follow_location(true);

//     auto res = cli.Get(url.c_str());

//     if (!res || res->status != 200)
//     {
//         return tl::make_unexpected(fmt::format("Failed to fetch release info: {}", res ? res->status : -1));
//     }

//     try
//     {
//         auto jsonResponse = nlohmann::json::parse(res->body);
//         if (!jsonResponse.contains("assets") || jsonResponse["assets"].empty())
//         {
//             return tl::make_unexpected("No assets found in the release.");
//         }
//         nlohmann::json const& assets = jsonResponse["assets"];
//         return assets;
//     }
//     catch (nlohmann::json::parse_error const& e)
//     {
//         return tl::make_unexpected(fmt::format("JSON parse error: {}", e.what()));
//     }
//     catch (std::exception& e)
//     {
//         return tl::make_unexpected(fmt::format("Error: {}", e.what()));
//     }
// }

auto get_coollab_download_url(nlohmann::basic_json<> const& release) -> std::string
{
    auto os_path = get_OS();
    for (const auto& asset : release)
    {
        std::string url = asset["browser_download_url"];
        if (url.find(os_path + ".zip") != std::string::npos)
            return url;
    }
}

auto coollab_version_is_installed(std::string_view const& version) -> bool
{
    return fs::exists(get_PATH() / version);
}