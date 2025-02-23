#include "LongPathsChecker.hpp"
#include "Cool/File/File.h"
#include "Cool/Task/TaskManager.hpp"
#include "Task_CheckForLongPathsEnabled.hpp"

#if defined(_WIN32)

void LongPathsChecker::check(std::filesystem::path const& path)
{
    if (_has_sent_notification)
        return;

    if (Cool::File::weakly_canonical(path).string().size() < 200)
        return;

    Cool::task_manager().submit(after(500ms), // Small delay to make sure users see it pop up and it draws their attention (if we send it as the app starts)
                                std::make_shared<Task_CheckForLongPathsEnabled>());
    _has_sent_notification = true;
}

#endif