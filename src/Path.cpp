#include "Path.hpp"
#include "Cool/Path/Path.h"

namespace Path {

auto installed_versions_folder() -> std::filesystem::path
{
#if defined(__APPLE__)
    return "~/Applications/Coollab"; // On MacOS, app bundles need to be installed in ~/Applications (or /Applications) for the permissions to work properly (e.g. accessing webcam and mic) . We do ~/Applications because it's user-specific and doesn't require admin privileges to write into it, unlike /Applications
#else
    return Cool::Path::user_data() / "Installed Versions";
#endif
}

auto projects_info_folder() -> std::filesystem::path
{
    return Cool::Path::user_data() / "Projects Info";
}

auto default_projects_folder() -> std::filesystem::path
{
    return Cool::Path::user_data() / "Projects";
}

auto versions_compatibility_file() -> std::filesystem::path
{
    return Cool::Path::user_data() / "versions_compatibility.txt";
}

} // namespace Path