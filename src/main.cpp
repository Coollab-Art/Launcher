#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>
#include "download.hpp"
#include "extractor.hpp"
#include "release.hpp"
#include "utils.hpp"

auto main() -> int
{
    tl::expected<std::vector<Release>, std::string> all_release;
    all_release = get_all_release();
    for (auto const& release : *all_release)
    {
        std::cout << release.name << " : " << release.download_url << std::endl;
    }
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
