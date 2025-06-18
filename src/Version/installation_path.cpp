#include "installation_path.hpp"
#include "Path.hpp"

auto installation_path(VersionName const& name) -> std::filesystem::path
{
    return Path::installed_versions_folder() / name.as_string_raw();
}

static auto exe_name() -> std::filesystem::path
{
#if defined(_WIN32)
    return "Coollab.exe";
#elif defined(__linux__)
    return "Coollab.AppImage";
#elif defined(__APPLE__)
    return "Coollab.app/Contents/MacOS/Coollab"; // We can't pass command-line args if we just open the app bundle, so we have to use the actual executable that is inside the bundle
#else
#error "Unsupported platform"
#endif
}

auto executable_path(VersionName const& name) -> std::filesystem::path
{
    return installation_path(name) / exe_name();
}