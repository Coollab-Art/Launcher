#pragma once
#include <vector>
#include <string_view>
#ifdef __APPLE__
#include <sys/stat.h> // chmod
#endif

auto extract_zip(std::vector<char> const& zip, std::string_view const& version) -> void;