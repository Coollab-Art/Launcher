#include "App.hpp"
#include "Cool/Dump/set_extra_dump_info.hpp"
#include "exe_path/exe_path.h"
//
#include "Cool/Core/run.h" // Must be included last otherwise it slows down compilation because it includes <ser20/archives/json.hpp>

class PathsConfig : public Cool::PathsConfig {
public:
    [[nodiscard]] auto user_data_shared() const -> std::filesystem::path override
    {
        return exe_path::user_data() / "Coollab"; // Use the same folder for Coollab and the Launcher, so that they share the same color theme and style settings
    }
};

auto main(int argc, char** argv) -> int
{
    Cool::set_extra_dump_info([](Cool::DumpStringGenerator& dump) {
        dump.add("OpenSSL",
#if CPPHTTPLIB_OPENSSL_SUPPORT
                 "Enabled"
#else
                 "Disabled"
#endif
        );
    });
    Cool::run<App, PathsConfig>(
        argc, argv,
        {
            .windows_configs = {{
                .maximize_on_startup_if = false,
            }},
        }
    );
}
