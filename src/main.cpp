#include <cstdlib>
#include <iostream>
#include "ReleaseManager.hpp"
#include "download.hpp"
#include "extractor.hpp"
#include "release.hpp"
#include "utils.hpp"

auto main() -> int
{
    try
    {
#ifdef __APPLE__
        // Install Homebrew & FFmpeg on MacOS
        install_macos_dependencies_if_necessary();
#endif

        ReleaseManager release_manager;
        release_manager.display_all_release();

        if (release_manager.no_release_installed()) // Aucune release d'installée
        {
            std::cout << "Aucune release d'installée. \nInstallation automatique de la dernière version : " << release_manager.get_latest_release().get_name() << std::endl;
            // get_latest_release().install();
        }
        else if (!release_manager.get_latest_release().is_installed()) // des versions installées mais pas la dernière
        {
            std::cout << "La dernière version : " << release_manager.get_latest_release().get_name() << " n'est pas installée, mais vous disposez des versions : ....." << std::endl;
            std::cout << "Souhaitez-vous installer la dernière version ? : " << release_manager.get_latest_release().get_name() << std::endl;
        }
        else // La dernière version est au minimum installé. On laisse le choix à l'utilisateur
        {
            std::cout << "Versions de Coollab disponibles :" << std::endl;
            release_manager.display_all_release();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
    }

    // std::string_view requested_version = "launcher-test-4";
    // // Install last Coollab release
    // if (!coollab_version_is_installed(requested_version))
    // {
    //     auto const release = get_release(requested_version);
    //     auto const zip     = download_zip(*release);
    //     extract_zip(*zip, requested_version);

    //     std::cout << "✅ Coollab " << requested_version << " is installed! ";
    // }
    // else
    //     std::cout << "❌ Coollab " << requested_version << " is already installed in : " << get_PATH() << std::endl;
}
