#include "suggest_dl_from_github.hpp"

static auto zip_name_to_manually_install_from_github() -> const char*
{
#if defined(_WIN32)
    return "Coollab-Windows-WithDLLs.zip";
#elif defined(__linux__)
    return "Coollab.AppImage";
#elif defined(__APPLE__)
    return "Coollab-MacOS.zip";
#else
#error "Unknown OS
#endif
}

auto suggest_dl_from_github() -> std::string
{
    return fmt::format("Try installing \"{}\" from [GitHub](https://github.com/Coollab-Art/Coollab/releases/latest) directly", zip_name_to_manually_install_from_github());
}