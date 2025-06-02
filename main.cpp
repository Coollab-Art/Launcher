#include "velopack/include/Velopack.hpp"
#include <cstddef>
#include <iostream>
#include <fstream>

static void update_app()
{
    try{
        Velopack::UpdateManager manager("http://localhost:8000/");
        std::wcout << L"Checking for updates..." << std::endl;

        auto updInfo = manager.CheckForUpdates();
        if (!updInfo.has_value()) {
            std::wcout << L"No updates found." << std::endl;
            return; // no updates available
        }

        std::wcout << L"Update found: " << updInfo->TargetFullRelease.Version.c_str() << std::endl;

        // download the update, optionally providing progress callbacks
        manager.DownloadUpdates(updInfo.value());
        std::wcout << L"Downloaded update." << std::endl;

        // prepare the Updater in a new process, and wait 60 seconds for this process to exit
        manager.WaitExitThenApplyUpdates(updInfo.value());
        std::wcout << L"Applying update..." << std::endl;

        exit(0); // exit the app to apply the update
    } 
    catch (const std::exception &ex) {
        std::cerr << "[EXCEPTION] " << ex.what() << "\n";
    } 
    catch (...) {
        std::cerr << "[EXCEPTION] Unknown exception\n";
    }
}

void LoggerCallback(void* user_data, const char* level, const char* message) {
    std::ofstream log("velopack_log.txt", std::ios::app);
    log << "[" << level << "] " << message << std::endl;
}

int wmain(int argc, wchar_t *argv[], wchar_t *envp[])
{
    // This should run as early as possible in the main method.
    // Velopack may exit / restart the app at this point. 
    // See VelopackApp class for more options/configuration.
    Velopack::VelopackApp::Build().SetLogger(LoggerCallback, nullptr).Run();

    std::wcout << L"App launched with arguments: ";
    for (int i = 0; i < argc; ++i) {
        std::wcout << argv[i] << L" ";
    }
    std::wcout << std::endl;

    // ... your other startup code here
    update_app();
    std::wcout << "Running version: 1.0.3\n" << std::endl;

    return 0;
}
