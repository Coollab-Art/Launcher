#pragma once
#include <mutex>
#include "VersionToUpgradeTo.hpp"
#include "parse_compatibility_file_line.hpp"

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
    friend class Task_FetchCompatibilityFile;
    void set_compatibility_entries(std::list<CompatibilityEntry>&& entries)
    {
        std::unique_lock lock{_mutex};
        _compatibility_entries = std::move(entries);
    }

private:
    std::list<CompatibilityEntry> _compatibility_entries; // TODO(Launcher) Use std::vector, psuh_back, and reverse when necessary
    mutable std::mutex            _mutex;
};

inline auto version_compatibility() -> VersionCompatibility&
{
    static auto instance = VersionCompatibility{};
    return instance;
}