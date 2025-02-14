#pragma once
#include "VersionName.hpp"
#include "VersionToUpgradeTo.hpp"

struct Incompatibility {};
struct SemiIncompatibility {
    std::string upgrade_instruction;
};
using CompatibilityEntry = std::variant<VersionName, SemiIncompatibility, Incompatibility>;

struct VersionNameAndUpgradeInstructions {
    VersionName              name;
    std::vector<std::string> upgrade_instructions;
};

class VersionCompatibility {
public:
    VersionCompatibility();
    auto compatible_versions(VersionName const&) const -> std::vector<VersionNameAndUpgradeInstructions>;
    auto version_to_upgrade_to_automatically(VersionName const&) const -> VersionToUpgradeTo;

private:
    std::list<CompatibilityEntry> _compatibility_entries;
};

inline auto version_compatibility() -> VersionCompatibility const&
{
    static auto instance = VersionCompatibility{};
    return instance;
}