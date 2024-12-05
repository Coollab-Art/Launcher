#include "App.hpp"
//
#include <Cool/Core/run.h> // Must be included last otherwise it slows down compilation because it includes <ser20/archives/json.hpp>

auto main(int argc, char** argv) -> int
{
    Cool::run<App>(
        argc, argv,
        {
            .windows_configs   = {{
                  .title                  = "Coollab Launcher",
                  .maximize_on_startup_if = false,
            }},
            .imgui_ini_version = 0,
        }
    );
}
