#include <Cool/Core/run.h> // Must be included last otherwise it slows down compilation because it includes <ser20/archives/json.hpp>
#include "App.hpp"
#include "exe_path/exe_path.h"

// Velopack includes
#include <codecvt>
#include <fstream>
#include <iostream>
#include <locale>
#include "../velopack/include/Velopack.hpp"

class PathsConfig : public Cool::PathsConfig {
public:
    [[nodiscard]] auto user_data_shared() const -> std::filesystem::path override
    {
        return exe_path::user_data() / "Coollab"; // Use the same folder for Coollab and the Launcher, so that they share the same color theme and style settings
    }
};

static void update_app()
{
    try
    {
        Velopack::UpdateManager manager("https://coollab-art.github.io/Launcher-Velopack-Tests");
        std::wcout << L"Checking for updates..." << std::endl;

        auto updInfo = manager.CheckForUpdates();
        if (!updInfo.has_value())
        {
            std::wcout << L"No updates found." << std::endl;
            return;
        }

        std::wcout << L"Update found: " << updInfo->TargetFullRelease.Version.c_str() << std::endl;

        manager.DownloadUpdates(updInfo.value());
        std::wcout << L"Downloaded update." << std::endl;

        manager.WaitExitThenApplyUpdates(updInfo.value());
        std::wcout << L"Applying update..." << std::endl;

        exit(0); // Terminate so Velopack can restart
    }
    catch (const std::exception& ex)
    {
        std::cerr << "[EXCEPTION] " << ex.what() << "\n";
    }
    catch (...)
    {
        std::cerr << "[EXCEPTION] Unknown exception\n";
    }
}

void LoggerCallback(void* /*user_data*/, const char* level, const char* message)
{
    std::ofstream log("velopack_log.txt", std::ios::app);
    if (log.is_open())
        log << "[" << level << "] " << message << std::endl;
}

auto main(int argc, char** argv) -> int
{
    Velopack::VelopackApp::Build()
        .SetLogger(LoggerCallback, nullptr)
        .Run();

    std::wcout << L"App launched with arguments: ";
    for (int i = 0; i < argc; ++i)
    {
        std::wcout << std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().from_bytes(argv[i]) << L" ";
    }
    std::wcout << std::endl;

    std::wcout << L"Running version: 1.0.9\n"
               << std::endl;

    update_app();

    Cool::run<App, PathsConfig>(
        argc, argv,
        {
            .windows_configs = {{
                .maximize_on_startup_if = false,
            }},
        }
    );
}
