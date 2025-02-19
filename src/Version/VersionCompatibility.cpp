#include "VersionCompatibility.hpp"
#include <ranges>
#include "Cool/Task/TaskManager.hpp"
#include "Cool/Utils/overloaded.hpp"
#include "LauncherSettings.hpp"
#include "Path.hpp"
#include "Task_FetchCompatibilityFile.hpp"
#include "VersionName.hpp"
#include "parse_compatibility_file_line.hpp"

VersionCompatibility::VersionCompatibility()
{
    auto ifs = std::ifstream{Path::versions_compatibility_file()};
    if (ifs.is_open())
    {
        auto line = std::string{};
        while (std::getline(ifs, line))
            parse_compatibility_file_line(line, _compatibility_entries);
    }

    Cool::task_manager().submit(std::make_shared<Task_FetchCompatibilityFile>()); // It's simpler to submit the task after parsing the file, it avoids concurrency if the fetch finishes before we finished parsing the file here
}

auto VersionCompatibility::compatible_versions(VersionName const& version_name) const -> std::vector<VersionNameAndUpgradeInstructions>
{
    std::unique_lock lock{_mutex};

    auto res                  = std::vector<VersionNameAndUpgradeInstructions>{};
    auto upgrade_instructions = std::vector<std::string>{};

    bool found{false};
    for (auto const& entry : _compatibility_entries | std::views::reverse)
    {
        bool do_break{false};
        std::visit(
            Cool::overloaded{
                [&](VersionName const& ver) {
                    if (found)
                    {
                        if (!ver.is_experimental() || launcher_settings().show_experimental_versions)
                            res.emplace_back(ver, upgrade_instructions);
                    }
                    else
                    {
                        if (ver == version_name)
                            found = true;
                    }
                },
                [&](SemiIncompatibility const& semi_incompatibility) {
                    if (found)
                        upgrade_instructions.push_back(semi_incompatibility.upgrade_instruction);
                },
                [&](Incompatibility) {
                    if (found)
                        do_break = true;
                },
            },
            entry
        );

        if (do_break)
            break;
    }

    return res;
}

auto VersionCompatibility::version_to_upgrade_to_automatically(VersionName const& version_name) const -> VersionToUpgradeTo
{
    std::unique_lock lock{_mutex};

    auto res = VersionToUpgradeTo{DontUpgrade{}};

    bool found{false};
    for (auto const& entry : _compatibility_entries | std::views::reverse)
    {
        bool do_break{false};
        std::visit(
            Cool::overloaded{
                [&](VersionName const& ver) {
                    if (found)
                    {
                        if (!ver.is_experimental() || launcher_settings().show_experimental_versions)
                            res = ver;
                    }
                    else
                    {
                        if (ver == version_name)
                            found = true;
                    }
                },
                [&](SemiIncompatibility const&) {
                    if (found)
                        do_break = true;
                },
                [&](Incompatibility) {
                    if (found)
                        do_break = true;
                },
            },
            entry
        );

        if (do_break)
            break;
    }

    return res;
}