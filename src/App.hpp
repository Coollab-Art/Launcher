#pragma once
#include "Cool/AppManager/IApp.h"
#include "Cool/DebugOptions/DebugOptions.h"
#include "Cool/DebugOptions/DebugOptionsManager.h"
#include "Cool/View/ViewsManager.h"
#include "Cool/Window/Window.h"
#include "Cool/Window/WindowManager.h"
#include "LauncherSettings.hpp"
#include "Path.hpp"
#include "Project/ProjectManager.hpp"
#include "Version/VersionRef.hpp"

using DebugOptionsManager = Cool::DebugOptionsManager<Cool::DebugOptions>;

class App : public Cool::IApp {
public:
    App(Cool::WindowManager& windows, Cool::ViewsManager& /* views */);

    void init() override;
    void update() override;
    void imgui_windows() override;
    void imgui_menus() override;
    auto wants_to_show_menu_bar() const -> bool override { return true; }
    void on_shutdown() override;

private:
    void launch(Project const& project);
    void launch(std::filesystem::path const& project_file_path);

private:
    ProjectManager        _project_manager{};
    VersionRef            _version_to_use_for_new_project{LatestInstalledVersion{}};
    Cool::Window&         _window; // NOLINT(*avoid-const-or-ref-data-members)
    std::filesystem::path _projects_folder{};

private:
    void save_to_json(nlohmann::json& json) const override
    {
        Cool::json_set(json, "Projects folder", _projects_folder);
    }
    void load_from_json(nlohmann::json const& json) override
    {
        Cool::json_get(json, "Projects folder", _projects_folder);
    }
};

// #include <cstdlib>
// #include <iostream>
// // #include "ProjectManager/path_to_project_to_open_on_startup.hpp"
// #include <fstream>
// #include <string>
// #include "VersionManager.hpp"
// #include "boxer/boxer.h"

// auto main(int argc, char** argv) -> int
// {
//     // TODO : faire une fonction!
//     std::string version;
//     if (argc > 1)
//     {
//         std::ifstream infile(argv[1]);
//         if (infile.is_open())
//             std::getline(infile, version);
//         infile.close();
//     }

//     try
//     {
// #ifdef __APPLE__
//         // Install Homebrew & FFmpeg on MacOS
//         install_macos_dependencies_if_necessary();
// #endif

//         VersionManager version_manager;
//         // version_manager.display_all_version();

//         Version const* version_to_launch = nullptr;

//         // L'utilisateur a déjà un projet sur une version différente de la latest
//         // Il a le choix de poursuivre son projet sur sa version, ou de télécharger la latest.
//         version_to_launch = version_manager.find_version(version);
//         if (version_to_launch == nullptr)
//         {
//             // std::cerr << "La version " << version << " n'a pas pu être installée, installation de la dernière version (par défaut).";
//             version_to_launch = version_manager.get_latest_version();
//             if (version_to_launch == nullptr)
//             {
//                 boxer::show("Please connect to the Internet so that we can install the latest version of Coollab.\nYou don't have any version installed yet.", "You are offline");
//                 return 0;
//             }
//         }

//         // Si la latest n'est pas installée -> on l'installe
//         if (!version_to_launch->is_installed())
//             version_manager.install_version(*version_to_launch);

//         version_manager.launch_version(*version_to_launch);
//     }
//     catch (const std::exception& e)
//     {
//         std::cerr << "Exception occurred: " << e.what() << std::endl;
//     }
// }
