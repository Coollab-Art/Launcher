#pragma once
#include <string>
#include <tl/expected.hpp>
#include "Release.hpp"

auto install_macos_dependencies_if_necessary() -> void;

// install essential packages
auto install_homebrew() -> void;
auto install_ffmpeg() -> void;

// download zip and return it
auto download_zip(const Release& release, std::atomic<float>& progression) -> tl::expected<std::string, std::string>;