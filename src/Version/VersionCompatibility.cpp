#include "VersionCompatibility.hpp"
#include "Cool/String/String.h"
#include "Cool/Utils/overloaded.hpp"
#include "LauncherSettings.hpp"
#include "VersionName.hpp"

VersionCompatibility::VersionCompatibility()
{
    auto ifs = std::ifstream{"C:/Dev/Cool/launcher/Lab/versions_compatibility.txt"}; // TODO(Launcher)

    auto line = std::string{};
    while (std::getline(ifs, line))
    {
        if (line.starts_with("---"))
        {
            auto const text = Cool::String::substring(line, 3, line.size());
            if (text.empty())
                _compatibility_entries.push_front(Incompatibility{});
            else
                _compatibility_entries.push_front(SemiIncompatibility{text});
        }
        else
        {
            auto const version_name = VersionName::from(line);
            if (!version_name.has_value())
            {
                assert(false);
                continue;
            }
            _compatibility_entries.push_front(*version_name);
        }
    }
}

auto VersionCompatibility::compatible_versions(VersionName const& version_name) const -> std::vector<VersionNameAndUpgradeInstructions>
{
    auto res                  = std::vector<VersionNameAndUpgradeInstructions>{};
    auto upgrade_instructions = std::vector<std::string>{};

    bool found{false};
    for (auto const& entry : _compatibility_entries)
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
    auto res = VersionToUpgradeTo{DontUpgrade{}};

    bool found{false};
    for (auto const& entry : _compatibility_entries)
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