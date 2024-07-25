#include "release.hpp"
#include <string>
#include <tl/expected.hpp>
#include "fmt/core.h"
#include "httplib.h"
#include "utils.hpp"
auto get_all_release() -> tl::expected<nlohmann::json, std::string>
{
    std::string     url = "https://api.github.com/repos/CoolLibs/Lab/releases";
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
        for (const auto& release : jsonResponse)
            if (!release["prerelease"])
            {
                for (const auto& asset : release["assets"])
                {
                    if (!(std::string(asset["browser_download_url"]).find(get_OS() + ".zip") != std::string::npos))
                        continue;
                    std::cout << release["name"] << " -> " << asset["name"] << " is ready to be installed! " << "\n";
                }
            }
        return jsonResponse;
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