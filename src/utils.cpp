#include "utils.hpp"
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "exe_path/exe_path.h"

// which OS ?
auto get_OS() -> std::string
{
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

// get the correct path for the installation
auto get_PATH() -> std::filesystem::path
{
    return exe_path::user_data() / "Coollab-Launcher";
}

// check if the download url is targeting a zip file.
auto is_zip_download(const nlohmann::basic_json<>& asset) -> bool
{
    return (std::string(asset["browser_download_url"]).find(get_OS() + ".zip") != std::string::npos);
}

// parse date
auto parse_date(const std::string& date_str) -> std::tm
{
    std::tm            tm = {};
    std::istringstream ss(date_str);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    if (ss.fail())
        throw std::runtime_error("Parse failed");
    return tm;
}

