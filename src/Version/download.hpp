#pragma once
#include <string>
#include <tl/expected.hpp>
#include "Version.hpp"

auto install_macos_dependencies_if_necessary() -> void;

// install essential packages
auto install_homebrew() -> void;
auto install_ffmpeg() -> void;