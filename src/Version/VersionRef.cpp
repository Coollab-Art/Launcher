#include "VersionRef.hpp"
#include "Cool/Utils/overloaded.hpp"

auto as_string_pretty(VersionRef const& version_ref) -> std::string
{
    return std::visit(
        Cool::overloaded{
            [&](LatestVersion) {
                return "latest version"s;
            },
            [&](LatestInstalledVersion) {
                return "latest installed version"s;
            },
            [](VersionName const& name) {
                return name.as_string_pretty();
            }
        },
        version_ref
    );
}