#include "Project.hpp"
#include <exception>
#include "Cool/File/File.h"
#include "Version/VersionName.hpp"
#include "handle_error.hpp"

void Project::init()
{
    // TODO(Launcher) Check that uuid matches the one in the file
}

auto Project::name() const -> std::string
{
    return Cool::File::file_name_without_extension(_file_path).string();
}

auto Project::version() const -> VersionName const&
{
    return _coollab_version.get_value([&]() {
        auto file = std::ifstream{file_path()};
        if (!file.is_open())
        {
            // TODO(Launcher) Handler error:   std::cerr << "Error: " << strerror(errno) << std::endl;
            return VersionName{"1.0.0"}; // TODO(Launcher) better default version
        }
        auto version = ""s;
        std::getline(file, version);
        return VersionName{version};
    });
}

auto Project::time_of_last_change() const -> std::filesystem::file_time_type const&
{
    return _time_of_last_change.get_value([&]() {
        try
        {
            return std::filesystem::last_write_time(thumbnail_path());
        }
        catch (std::exception const& e)
        {
            handle_error(e.what());
            return std::filesystem::file_time_type{};
        }
    });
}