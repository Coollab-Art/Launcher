#pragma once
#include "VersionName.hpp"

enum class InstallationStatus : uint8_t {
    NotInstalled,
    Installing,
    Installed,
};

struct Version {
    VersionName                name;
    InstallationStatus         installation_status{};
    std::optional<std::string> download_url{};
    std::optional<std::string> changelog_url{};

    friend auto operator<=>(Version const& a, Version const& b) { return b.name <=> a.name; } // Compare b to a and not the other way around because when sorting or vector of Version, we want the latest to be at the front
    friend auto operator==(Version const& a, Version const& b) -> bool { return a.name == b.name; }
};