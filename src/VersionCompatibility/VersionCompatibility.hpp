#pragma once
#include <mutex>
#include "Version/VersionToUpgradeTo.hpp"
#include "parse_compatibility_file_line.hpp"

struct VersionNameAndUpgradeInstructions {
    VersionName              name;
    std::vector<std::string> upgrade_instructions;
};

class VersionCompatibility {
public:
    VersionCompatibility();
    auto compatible_and_semi_compatible_versions(VersionName const&) const -> std::vector<VersionNameAndUpgradeInstructions>;
    auto compatible_versions(VersionName const&) const -> std::vector<VersionName>;
    auto version_to_upgrade_to_automatically(VersionName const&) const -> VersionToUpgradeTo;

private:
    friend class Task_FetchCompatibilityFile;
    void set_compatibility_entries(std::vector<CompatibilityEntry>&& entries)
    {
        std::unique_lock lock{_mutex};
        _compatibility_entries = std::move(entries);
    }

private:
    std::vector<CompatibilityEntry> _compatibility_entries;
    mutable std::mutex              _mutex;
};

inline auto version_compatibility() -> VersionCompatibility&
{
    static auto instance = VersionCompatibility{};
    return instance;
}