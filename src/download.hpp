#pragma once
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include "nlohmann/json.hpp"

auto install_macos_dependencies_if_necessary() -> void;

// install essential packages
auto install_homebrew() -> void;
auto install_ffmpeg() -> void;


// download zip and return it
auto download_zip(nlohmann::basic_json<> const& release) -> tl::expected<std::string, std::string>;