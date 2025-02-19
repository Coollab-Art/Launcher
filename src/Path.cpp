#include "Path.hpp"
#include "Cool/Path/Path.h"

namespace Path {

auto installed_versions_folder() -> std::filesystem::path
{
    return Cool::Path::user_data() / "Installed Versions";
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