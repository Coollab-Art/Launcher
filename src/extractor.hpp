#pragma once
#include <filesystem>
#include <string_view>
#ifdef __APPLE__
#include <sys/stat.h> // chmod
#endif

auto extract_zip(std::string const& zip, std::filesystem::path const& installation_path) -> void;