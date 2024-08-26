#include <cstdlib>
#include <iostream>
// #include "ProjectManager/path_to_project_to_open_on_startup.hpp"
#include <fstream>
#include <string>
#include "ReleaseManager.hpp"
#include "download.hpp"

#define INTERACTIVE_LAUNCHER 0

auto main(int argc, char** argv) -> int
{
    // TODO : faire une fonction!
    std::string version;
    if (argc > 1)
    {
        std::ifstream infile(argv[1]);
        if (infile.is_open())
            std::getline(infile, version);
        infile.close();
    }

    try
    {
#ifdef __APPLE__
        // Install Homebrew & FFmpeg on MacOS
        install_macos_dependencies_if_necessary();
#endif

        ReleaseManager release_manager;

#if INTERACTIVE_LAUNCHER
        if (release_manager.no_release_installed()) // Aucune release d'installée
        {
            Release latest_release = release_manager.get_latest_release();
            release_manager.display_all_release();
            std::cout << "Aucune release d'installée. \nInstallation automatique de la dernière version : " << latest_release.get_name() << std::endl;
            int confirm = 0;
            std::cout << "Lancer l'installation : (1) ";
            std::cin >> confirm;
            if (confirm)
            {
                release_manager.install_release(latest_release);
                release_manager.launch_release(latest_release);
            }
        }
        else if (!release_manager.get_latest_release().is_installed()) // des versions installées mais pas la dernière
        {
            std::cout << "La dernière version : " << release_manager.get_latest_release().get_name() << " n'est pas installée, mais vous disposez des versions : ....." << std::endl;
            std::cout << "Souhaitez-vous installer la dernière version ? : " << release_manager.get_latest_release().get_name() << std::endl;
        }
        else // La dernière version est au minimum installé. On laisse le choix à l'utilisateur
        {
            int choice = 0;
            std::cout << "Voir les versions de Coollab : (1)" << std::endl;
            std::cout << "Installer une version : (2)" << std::endl;
            std::cout << "Lancer une version (3) :" << std::endl;
            std::cout << "Choix : ";
            std::cin >> choice;

            if (choice == 1)
                release_manager.display_all_release();
            else if (choice == 2)
            {
                std::string version_to_install;
                std::cout << "Choisissez le nom de la version à installer : ";
                std::cin >> version_to_install;

                for (const Release& release : release_manager.get_all_release())
                {
                    if (release.get_name() == version_to_install && !release.is_installed())
                        release_manager.install_release(release);
                    else if (release.get_name() == version_to_install)
                        std::cout << release.get_name() << " est déjà installé !" << std::endl;
                }
            }
            else if (choice == 3)
            {
                std::string version_to_launch;
                std::cout << "Choisissez le nom de la version à lancer : ";
                std::cin >> version_to_launch;

                for (const Release& release : release_manager.get_all_release())
                {
                    if (release.get_name() == version_to_launch && release.is_installed())
                        release_manager.install_release(release);
                    else if (release.get_name() == version_to_launch)
                        std::cout << release.get_name() << " n'est pas installé !" << std::endl;
                }
            }
        }
#else
        Release const* release_to_launch = nullptr;

        // L'utilisateur a déjà un projet sur une version différente de la latest
        // Il a le choix de poursuivre son projet sur sa version, ou de télécharger la latest.
        release_to_launch = release_manager.find_release(version);
        if (release_to_launch == nullptr)
        {
            std::cerr << "La version " << version << " n'a pas pu être installée, installation de la dernière version (par défaut).";
            release_to_launch = &release_manager.get_latest_release();
        }

        // Si la latest n'est pas installée -> on l'installe
        if (!release_to_launch->is_installed())
            release_manager.install_release(*release_to_launch);

        release_manager.launch_release(*release_to_launch);

        // Problème internet... (TODO)

#endif
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
    }
}
