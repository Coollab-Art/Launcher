#pragma once
#include "VersionName.hpp"

auto installation_path(VersionName const& name) -> std::filesystem::path;
auto executable_path(VersionName const& name) -> std::filesystem::path;