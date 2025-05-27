#pragma once
#include "VersionName.hpp"

struct LatestVersion {
    friend auto operator==(LatestVersion const&, LatestVersion const&) -> bool { return true; }
};
struct LatestInstalledVersion {
    friend auto operator==(LatestInstalledVersion const&, LatestInstalledVersion const&) -> bool { return true; }
};

using VersionRef = std::variant<
    LatestVersion,
    LatestInstalledVersion,
    VersionName>;

auto as_string(VersionRef const&) -> std::string;