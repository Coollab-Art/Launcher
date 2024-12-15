#pragma once
#include "VersionName.hpp"

/// NB: The version needs to be already installed
void launch(VersionName const&, std::optional<std::filesystem::path> const& project_file_path);