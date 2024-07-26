#include "utils.hpp"
#include <cstddef>
#include <iostream>
#include "exe_path/exe_path.h"

auto get_OS() -> std::string
{
    // which OS ?
#ifdef __APPLE__
    return "MacOS";
#elif _WIN32
    return "Windows";
#elif __linux__
    return "Linux";
#else
    static_assert(false);
#endif
}

auto get_PATH() -> std::filesystem::path
{
    return exe_path::user_data() / "Coollab-Launcher";
}

auto is_zip_download(const nlohmann::basic_json<>& asset) -> bool
{
    return (std::string(asset["browser_download_url"]).find(get_OS() + ".zip") != std::string::npos);
}