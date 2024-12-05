#include "Release.hpp"
#include <filesystem>
#include <string>
#include <tl/expected.hpp>
#include "Cool/spawn_process.hpp"
#include "fmt/core.h"
#include "handle_error.hpp"
#include "httplib.h"
#include "utils.hpp"

namespace fs = std::filesystem;

auto Release::installation_path() const -> std::filesystem::path
{
    return get_PATH() / this->get_name();
}

static auto exe_name() -> std::filesystem::path
{
#if defined(_WIN32)
    return "Coollab.exe";
#elif defined(__linux__) || defined(__APPLE__)
    return "Coollab";
#else
#error "Unsupported platform"
#endif
}

auto Release::executable_path() const -> std::filesystem::path
{
    return installation_path() / exe_name();
}

auto Release::is_installed() const -> bool
{
    return fs::exists(installation_path());
}

// get a single release with tag_name
auto get_release(std::string_view const& version) -> tl::expected<nlohmann::json, std::string>
{
    std::string url = fmt::format("https://api.github.com/repos/CoolLibs/Lab/releases/tags/{}", version);

    httplib::Client cli("https://api.github.com");
    cli.set_follow_location(true);

    auto res = cli.Get(url);

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
        return tl::make_unexpected(fmt::format("JSON parsing error: {}", e.what()));
    }
    catch (std::exception& e)
    {
        return tl::make_unexpected(fmt::format("{}", e.what()));
    }
}

void Release::launch() const
{
    std::cout << "Launching Coollab " << get_name() << '\n';
    handle_error(Cool::spawn_process(executable_path()));
}