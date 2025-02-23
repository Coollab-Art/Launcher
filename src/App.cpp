#include "App.hpp"
#include <imgui.h>
#include "COOLLAB_FILE_EXTENSION.hpp"
#include "Cool/CommandLineArgs/CommandLineArgs.h"
#include "Cool/DebugOptions/debug_options_windows.h"
#include "Cool/File/File.h"
#include "Cool/ImGui/ColorThemes.h"
#include "Cool/ImGui/ImGuiExtras.h"
#include "Cool/Log/ToUser.h"
#include "Cool/Task/TaskManager.hpp"
#include "ImGuiNotify/ImGuiNotify.hpp"
#include "LauncherSettings.hpp"
#include "Task_CheckForLongPathsEnabled.hpp"
#include "Version/VersionManager.hpp"
#include "Version/VersionRef.hpp"

App::App(Cool::WindowManager& windows, Cool::ViewsManager& /* views */)
    : _window{windows.main_window()}
{}

void App::init()
{
    // When double-clicking on a Coollab file, this will open the launcher and pass the path to that file as a command-line argument
    // We want to launch that project asap
    if (!Cool::command_line_args().get().empty())
        launch(Cool::command_line_args().get()[0]);

#if defined(_WIN32)
    if (_project_manager.has_some_projects()) // Don't show it the first time users open the launcher after installing it, because we don't want to scare them with something that might look like a virus
    {
        Cool::task_manager().submit(after(500ms), // Small delay to make sure users see it pop up and it draws their attention
                                    std::make_shared<Task_CheckForLongPathsEnabled>());
    }
#endif
}

void App::update()
{
    auto const& io = ImGui::GetIO();
    if (inputs_are_allowed() && !io.WantTextInput)
    {
        if (io.KeyCtrl && ImGui::IsKeyReleased(ImGuiKey_O))
            open_external_project();
        else if (io.KeyCtrl && ImGui::IsKeyReleased(ImGuiKey_N))
            open_new_project();
    }
}

void App::on_shutdown()
{
    launcher_settings().save(); // Even if the user doesn't change the settings, we will save the settings they have seen once, so that if a new version of the software comes with new settings, we will not change settings that the user is used to
}

static auto folder_path_error_message(std::string const& name) -> std::optional<std::string>
{
    for (char const invalid_char : {'.', '<', '>', '\"', '|', '?', '*', '\0'})
    {
        if (name.find(invalid_char) != std::string::npos)
            return fmt::format("Folder name cannot contain a {}\nChange it below", invalid_char);
    }
    {
        bool is_in_drive_part_of_the_path{Cool::File::is_absolute(name)};
        for (char const c : name)
        {
            if (c == '/' || c == '\\')
                is_in_drive_part_of_the_path = false;
            else if (c == ':' && !is_in_drive_part_of_the_path)
                return "Folder name cannot contain a :\nChange it below";
        }
    }

    if (name.ends_with(' '))
        return "Folder name cannot end with a space\nChange it below";

    if (name.starts_with("--"))
        return "Folder name cannot start with --\nChange it below"; // Otherwise, when passing this folder name as a command-line argument, we would think it's an argument and not a folder name

    {
        auto const check = [](std::string const& name) -> std::optional<std::string> {
            auto upper_case_name = name;
            std::transform(upper_case_name.begin(), upper_case_name.end(), upper_case_name.begin(), [](char c) {
                return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); // We need those static_casts to avoid undefined behaviour, cf. https://en.cppreference.com/w/cpp/string/byte/toupper
            });
            for (const char* invalid_name : {
                     "CON", "PRN", "AUX", "NUL",
                     "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
                     "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
                 })
            {
                if (upper_case_name == invalid_name)
                    return fmt::format("{} is a reserved name\nChange it below", name);
            }
            return std::nullopt;
        };
        auto one_folder_name = std::string{};
        for (char const c : name)
        {
            if (c == '/' || c == '\\')
            {
                auto const err = check(one_folder_name);
                if (err.has_value())
                    return err;
                one_folder_name = "";
            }
            else
                one_folder_name += c;
        }
        auto const err = check(one_folder_name);
        if (err.has_value())
            return err;
    }

    return std::nullopt;
}

void App::imgui_windows()
{
    Cool::Log::ToUser::console().imgui_window();
    Cool::debug_options_windows(nullptr, _window);
    { // Versions
        ImGui::Begin("Versions");
        version_manager().imgui_manage_versions();
        ImGui::End();
    }

    { // New Project
        ImGui::Begin("New Project");

        auto const err = folder_path_error_message(_projects_folder.string());

        Cool::ImGuiExtras::disabled_if(err, [&]() {
            if (Cool::ImGuiExtras::colored_button("New Project", Cool::color_themes()->editor().get_color("Accent")))
                open_new_project();
        });
        ImGui::SameLine();
        version_manager().imgui_versions_dropdown(_version_to_use_for_new_project);

        if (err.has_value())
            Cool::ImGuiExtras::warning_text(err->c_str());
        Cool::ImGuiExtras::folder("", &_projects_folder);
        ImGui::SetItemTooltip("%s", "Folder where the new project will be saved.\nIf left empty or relative, it will be relative to the launcher's User Data folder");

        ImGui::End();
    }

    {
        ImGui::Begin("Projects");

        if (ImGui::Button("Open external project"))
            open_external_project();
        ImGui::SetItemTooltip("Browse your files to open a project that does not appear in the list of projects below");

        ImGui::BeginChild("##projects_list"); // Child window to make sure the "Open external project" button stays at the top, and the scrollbar only affects the list of projects
        _project_manager.imgui([&](Project const& project) { launch(project); });
        ImGui::EndChild();
        ImGui::End();
    }

    // ImGui::ShowDemoWindow();
}

void App::imgui_menus()
{
    if (ImGui::BeginMenu("Settings"))
    {
        launcher_settings().imgui();
        ImGui::EndMenu();
    }

    ImGui::SetCursorPosX( // HACK while waiting for ImGui to support right-to-left layout. See issue https://github.com/ocornut/imgui/issues/5875
        ImGui::GetWindowSize().x
        - ImGui::CalcTextSize("Debug").x
        - 3.f * ImGui::GetStyle().ItemSpacing.x
        - ImGui::GetStyle().WindowPadding.x
    );
    if (ImGui::BeginMenu("Debug"))
    {
        DebugOptionsManager::imgui_ui_for_all_options();
        ImGui::EndMenu();
    }
}

void App::open_external_project()
{
    auto const path = Cool::File::file_opening_dialog({
        .file_filters   = {{"Coollab project", COOLLAB_FILE_EXTENSION}},
        .initial_folder = {},
    });
    if (path.has_value())
        launch(*path);
}

void App::open_new_project()
{
    version_manager().install_ifn_and_launch(_version_to_use_for_new_project, FolderToCreateNewProject{_projects_folder});
}

void App::launch(Project const& project)
{
    auto const version = project.version_to_launch();
    if (!version.has_value())
    {
        ImGuiNotify::send({
            .type    = ImGuiNotify::Type::Error,
            .title   = "Can't open project",
            .content = "Unknown version",
        });
        return;
    }
    version_manager().install_ifn_and_launch(*version, FileToOpen{project.file_path()});
}

void App::launch(std::filesystem::path const& project_file_path)
{
    launch(Project{project_file_path});
}