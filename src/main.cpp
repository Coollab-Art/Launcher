#include "App.hpp"
//
#include <Cool/Core/run.h> // Must be included last otherwise it slows down compilation because it includes <ser20/archives/json.hpp>

auto main(int argc, char** argv) -> int
{
    // TODO(Launcher) Handle command line args to directly launch a project
    // NB: maybe we should still show Cool window to show progress bar while we install a version

    // if (argc > 1)
    // {
    //     if (argv[1] == "--new_project"sv)
    //     {
    //         auto const* maybe_release = get_latest_release();
    //         if (!maybe_release)
    //         {
    //             boxer::show(
    //                 "Please connect to the Internet so that we can install the latest version of Coollab.\nYou don't have any version installed yet.",
    //                 "You are offline"
    //             );
    //             return EXIT_FAILURE;
    //         }
    //         maybe_release->launch();
    //         return EXIT_SUCCESS;
    //     }
    //     else
    //     {
    //         std::ifstream infile(argv[1]);
    //         if (infile.is_open())
    //         {
    //             std::string version;
    //             std::getline(infile, version);
    //             //TODO(Launcher) Update the RecentlyOpened list
    //         }
    //     }
    // }

    Cool::run<App>(
        argc, argv,
        {
            .windows_configs   = {{
                  .maximize_on_startup_if = false,
            }},
            .imgui_ini_version = 0,
        }
    );
}
