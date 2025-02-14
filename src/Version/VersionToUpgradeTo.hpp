#pragma once
#include "VersionName.hpp"

struct DontUpgrade {
    friend auto operator<=>(DontUpgrade, DontUpgrade) = default;
};

using VersionToUpgradeTo = std::variant<VersionName, DontUpgrade>;