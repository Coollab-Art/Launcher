#pragma once
#include <sys/stat.h>
#include <ctime>
#include <filesystem>
#include <string>
#include "nlohmann/json.hpp"

// get the OS
auto get_OS() -> std::string;
auto get_PATH() -> std::filesystem::path;
auto is_zip_download(const std::string& download_url) -> bool;
