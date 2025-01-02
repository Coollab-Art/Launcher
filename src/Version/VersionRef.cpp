#include "VersionRef.hpp"
#include "wcam/src/overloaded.hpp"

auto as_string(VersionRef const& version_ref) -> std::string
{
    return std::visit(
        wcam::overloaded{
            [&](LatestVersion) {
                return "latest version"s;
            },
            [&](LatestInstalledVersion) {
                return "latest installed version"s;
            },
            [](VersionName const& name) {
                return name.as_string();
            }
        },
        version_ref
    );
}