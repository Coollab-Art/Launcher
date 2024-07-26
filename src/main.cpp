#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>
#include "download.hpp"
#include "extractor.hpp"
#include "release.hpp"
#include "release_manager.hpp"
#include "utils.hpp"

auto main() -> int
{
    // install all necessary dependencies

    ReleaseManager release_manager;
    release_manager.display_all_release_available();

    if (release_manager.all_release_installed.empty()) // no release install
        release_manager.install_release(true);         //     install latest release

    // everytime user use the launcher -> set the requested version as latest one
    // if (no_release_installed)

    // if (!no_release_installed() && !latest_release_installed())
    //     propose to install latest_release

    //     try
    //     {
    // #ifdef __APPLE__
    //         // Install Homebrew & FFmpeg on MacOS
    //         install_macos_dependencies_if_necessary();
    // #endif

    //         std::string_view requested_version = "launcher-test-4";
    //         // Install last Coollab release
    //         if (!coollab_version_is_installed(requested_version))
    //         {
    //             auto const release = get_release(requested_version);
    //             auto const zip     = download_zip(*release);
    //             extract_zip(*zip, requested_version);

    //             std::cout << "✅ Coollab " << requested_version << " is installed! ";
    //         }
    //         else
    //             std::cout << "❌ Coollab " << requested_version << " is already installed in : " << get_PATH() << std::endl;
    //     }
    //     catch (const std::exception& e)
    //     {
    //         std::cerr << "Exception occurred: " << e.what() << std::endl;
    //     }
}
