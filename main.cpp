#include "velopack/include/Velopack.hpp"
#include <iostream>

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

int wmain(int argc, wchar_t *argv[], wchar_t *envp[])
{
    // This should run as early as possible in the main method.
    // Velopack may exit / restart the app at this point. 
    // See VelopackApp class for more options/configuration.
    Velopack::VelopackApp::Build().Run();

    // ... your other startup code here
    std::wcout << "Running version: 1.0.0\n" << std::endl;
    update_app();

    return 0;
}
