#include "Project.hpp"
#include "Cool/File/File.h"

void Project::init()
{
    _name = Cool::File::file_name_without_extension(_file_path).string();
    // TODO(Launcher) Grab version from file
}