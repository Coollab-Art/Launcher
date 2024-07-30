#pragma once
#include <string_view>
#ifdef __APPLE__
#include <sys/stat.h> // chmod
#endif

auto extract_zip(std::string const& zip, std::string_view const& version) -> void;