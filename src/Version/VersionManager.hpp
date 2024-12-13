#pragma once
#include <tl/expected.hpp>
#include "Version.hpp"
#include "VersionName.hpp"

class VersionManager {
public:
    VersionManager();

    [[nodiscard]] auto find(VersionName const& version) const -> Version const*;
    [[nodiscard]] auto latest() const -> Version const*;

    auto no_version_installed() -> bool;

    void imgui();

private:
    std::vector<Version> _versions; // Sorted, from latest to oldest version
};