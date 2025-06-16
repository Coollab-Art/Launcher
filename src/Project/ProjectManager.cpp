#include "ProjectManager.hpp"
#include <filesystem>
#include <stack>
#include <vector>
#include "COOLLAB_FILE_EXTENSION.hpp"
#include "Cool/File/File.h"
#include "Cool/File/PathChecks.hpp"
#include "Cool/ImGui/Fonts.h"
#include "Cool/ImGui/ImGuiExtras.h"
#include "Cool/TextureSource/TextureLibrary_Image.h"
#include "Cool/TextureSource/default_textures.h"
#include "Cool/Utils/overloaded.hpp"
#include "ImGuiNotify/ImGuiNotify.hpp"
#include "LongPaths/LongPathsChecker.hpp"
#include "Path.hpp"
#include "Project.hpp"
#include "boxer/boxer.h"
#include "imgui.h"
#include "open/open.hpp"

class UndoAction {
public:
    virtual void undo()   = 0;
    virtual ~UndoAction() = default;
};

class UndoDeleteProjectAction : public UndoAction {
    std::filesystem::path backup_folder;
    std::filesystem::path original_folder;
    std::filesystem::path original_file;
    Project               restored_project;

public:
    UndoDeleteProjectAction(const std::filesystem::path& info_path, const std::filesystem::path& file_path)
        : original_folder(info_path), original_file(file_path), restored_project(file_path) // Save project object to re-add
    {
        backup_folder = Cool::Path::user_data() / "project_backup" / std::to_string(std::time(nullptr));
        std::filesystem::create_directories(backup_folder);

        std::filesystem::copy(info_path, backup_folder / "info_folder", std::filesystem::copy_options::recursive);
        std::filesystem::copy(file_path, backup_folder / "project_file");
    }

    Project undo_and_return_project()
    {
        std::filesystem::copy(backup_folder / "info_folder", original_folder, std::filesystem::copy_options::recursive);
        std::filesystem::copy(backup_folder / "project_file", original_file, std::filesystem::copy_options::overwrite_existing);
        std::cout << "Restored deleted project from backup." << std::endl;
        return restored_project;
    }

    void undo() override
    {
        // Not used anymore, fallback
        undo_and_return_project();
    }
};

std::stack<std::unique_ptr<UndoAction>> undo_stack;

ProjectManager::ProjectManager()
{
    try
    {
        for (auto const& entry : std::filesystem::directory_iterator{Path::projects_info_folder()})
        {
            if (!entry.is_directory())
            {
                assert(false);
                continue;
            }

            auto file = std::ifstream{entry.path() / "path.txt"};
            if (!file.is_open())
            {
                // TODO(Launcher) error
                continue;
            }
            std::string path;
            std::getline(file, path);
            _projects.emplace_back(path);
#if defined(_WIN32)
            long_paths_checker().check(path);
#endif
        }
    }
    catch (std::exception const&)
    {
        // TODO(Launcher) error
        // return fmt::format("{}", e.what());
    }
    std::sort(_projects.begin(), _projects.end(), [](Project const& a, Project const& b) {
        return a.time_of_last_change() > b.time_of_last_change();
    });
}

static auto project_name_error_message(std::string const& name, std::filesystem::path const& new_path, std::filesystem::path const& current_path) -> std::optional<std::string>
{
    if (Cool::File::exists(new_path) && !Cool::File::equivalent(new_path, current_path))
        return "Name already used by another project";

    if (name.empty())
        return "Name cannot be empty";

    for (char const invalid_char : {'.', '<', '>', ':', '\"', '/', '\\', '|', '?', '*', '\0'})
    {
        if (name.find(invalid_char) != std::string::npos)
            return fmt::format("Name cannot contain a {}", invalid_char);
    }

    if (name.ends_with(' '))
        return "Name cannot end with a space";

    if (name.starts_with("--"))
        return "Name cannot start with --"; // Otherwise, when passing this file name as a command-line argument, we would think it's an argument and not a file name

    {
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
                return fmt::format("{} is a reserved name", name);
        }
    }

    return std::nullopt;
}

void ProjectManager::imgui(std::function<void(Project const&)> const& launch_project)
{
    auto project_to_remove = _projects.end();
    auto project_to_add    = std::optional<Project>{};

    std::optional<Project> restored_project{};

    if (ImGui::IsKeyPressed(ImGuiKey_Z) && ImGui::GetIO().KeyCtrl)
    {
        if (!undo_stack.empty())
        {
            auto* action = dynamic_cast<UndoDeleteProjectAction*>(undo_stack.top().get());
            if (action != nullptr)
            {
                restored_project = action->undo_and_return_project();
            }
            else
            {
                undo_stack.top()->undo(); // fallback for other undo types
            }
            undo_stack.pop();
        }
    }

    for (auto it = _projects.begin(); it != _projects.end(); ++it)
    {
        // Items that are outside of the view can be "clipped". We still need to render them because otherwise ImGui doesn't know the
        // size of the whole dropdown list. But we can skip loading the texture, which is the only thing that actually costs some performance anyways.
        bool const is_visible = ImGui::GetWindowPos().y - 1000.f < ImGui::GetCursorScreenPos().y
                                && ImGui::GetCursorScreenPos().y < ImGui::GetWindowPos().y + ImGui::GetWindowSize().y;

        auto& project = *it;
        ImGui::PushID(&project);
        ImGui::PushFont(Cool::Font::bold());
        ImGui::SeparatorText(project.name().c_str());
        ImGui::PopFont();

        auto const rename_popup_id = ImGui::GetID("##rename");

        auto const widget = [&]() {
            Cool::Texture const* thumbnail = is_visible ? Cool::TextureLibrary_Image::instance().get(project.thumbnail_path(), 1s) : &Cool::dummy_texture();
            if (thumbnail)
            {
                Cool::ImGuiExtras::image_framed(thumbnail->imgui_texture_id(), {100.f, 100.f}, {
                                                                                                   .frame_thickness       = 4.f,
                                                                                                   .background_texture_id = _checkerboard_texture.get({100, 100}).imgui_texture_id(),
                                                                                               });
                ImGui::SameLine();
            }
            ImGui::BeginGroup();
            ImGui::TextUnformatted(project.file_path().string().c_str());
            if (project.current_version().has_value())
                ImGui::TextUnformatted(project.current_version()->as_string().c_str());
            else if (project.file_not_found())
                Cool::ImGuiExtras::warning_text("Project file not found");
            else
                Cool::ImGuiExtras::warning_text("Unknown version");
            std::visit(
                Cool::overloaded{
                    [](VersionName const& version_name) {
                        ImGui::SameLine();
                        ImGui::Text("(Will be upgraded to %s)", version_name.as_string().c_str());
                    },
                    [&](DontUpgrade) {},
                },
                project.version_to_upgrade_to()
            );
            if (project.file_not_found())
            {
                if (ImGui::Button("Find project file"))
                {
                    auto const path = Cool::File::file_opening_dialog({
                        .file_filters   = {{"Coollab project", COOLLAB_FILE_EXTENSION}},
                        .initial_folder = Cool::File::find_closest_existing_folder(project.file_path()),
                    });
                    if (path.has_value())
                    {
                        std::optional<std::string> project_with_same_path{};
                        for (auto const& proj : _projects)
                        {
                            if (&proj != &project && Cool::File::equivalent(proj.file_path(), *path))
                            {
                                project_with_same_path = proj.name();
                                break;
                            }
                        }
                        if (!project_with_same_path.has_value())
                        {
                            auto const old_info_folder_path = project.info_folder_path();
                            project.set_file_path(*path);
                            Cool::File::rename(old_info_folder_path, project.info_folder_path());
                            Cool::File::set_content(project.info_folder_path() / "path.txt", Cool::File::weakly_canonical(*path).string());
#if defined(_WIN32)
                            long_paths_checker().check(project.file_path());
#endif
                        }
                        else
                        {
                            ImGuiNotify::send({
                                .type    = ImGuiNotify::Type::Warning,
                                .title   = fmt::format("Invalid path \"{}\"", Cool::File::weakly_canonical(*path)),
                                .content = fmt::format("\"{}\" already uses this path. You cannot assign it to \"{}\"", *project_with_same_path, project.name()),
                            });
                        }
                    }
                }
            }
            ImGui::EndGroup();
        };
        if (project.file_not_found())
        {
            ImGui::BeginGroup();
            widget();
            ImGui::EndGroup();
        }
        else
        {
            if (Cool::ImGuiExtras::big_selectable(widget))
                launch_project(project);
        }
        if (ImGui::BeginPopupContextItem("##project_context_menu"))
        {
            Cool::ImGuiExtras::disabled_if(project.file_not_found(), "File not found", [&]() {
                if (ImGui::Selectable("Make a copy", false, ImGuiSelectableFlags_SpanAllColumns /* HACK to work around a bug in ImGui (https://github.com/ocornut/imgui/issues/8203)*/))
                {
                    auto const new_path = Cool::File::find_available_path(project.file_path(), Cool::PathChecks{});
                    Cool::File::copy_file(project.file_path(), new_path);

                    project_to_add = Project{new_path};

                    Cool::File::copy_file(project.info_folder_path() / "thumbnail.png", project_to_add->info_folder_path() / "thumbnail.png");
                    Cool::File::set_content(project_to_add->info_folder_path() / "path.txt", Cool::File::weakly_canonical(new_path).string());
#if defined(_WIN32)
                    long_paths_checker().check(project_to_add->file_path());
#endif
                }
                if (ImGui::Selectable("Rename", false, ImGuiSelectableFlags_SpanAllColumns /* HACK to work around a bug in ImGui (https://github.com/ocornut/imgui/issues/8203)*/))
                    ImGui::OpenPopup(rename_popup_id);
            });
            if (ImGui::Selectable("Delete project"))
            {
                if (boxer::Selection::OK == boxer::show("Are you sure? This cannot be undone", fmt::format("Deleting project \"{}\"", project.name()).c_str(), boxer::Style::Warning, boxer::Buttons::OKCancel))
                {
                    // Backup before deleting
                    undo_stack.push(std::make_unique<UndoDeleteProjectAction>(
                        project.info_folder_path(), project.file_path()
                    ));

                    Cool::File::remove_folder(project.info_folder_path());
                    Cool::File::remove_file(project.file_path());
                    project_to_remove = it;
                }
            }
            if (ImGui::Selectable("Remove from list"))
            {
                if (boxer::Selection::OK == boxer::show("Are you sure? This will not delete the project file", fmt::format("Removing project \"{}\" from the list", project.name()).c_str(), boxer::Style::Warning, boxer::Buttons::OKCancel))
                {
                    Cool::File::remove_folder(project.info_folder_path());
                    project_to_remove = it;
                }
            }

            project.imgui_version_to_upgrade_to();

            ImGui::SeparatorText("");

            if (ImGui::Selectable("Reveal in File Explorer"))
            {
                if (project.file_not_found())
                    Cool::open_folder_in_explorer(Cool::File::without_file_name(project.file_path()));
                else
                    Cool::open_focused_in_explorer(project.file_path());
            }
            if (ImGui::Selectable("Copy file path"))
            {
                ImGui::SetClipboardText(project.file_path().string().c_str());
            }
#if DEBUG
            if (ImGui::Selectable("DEBUG: Open info folder"))
            {
                Cool::open_folder_in_explorer(project.info_folder_path());
            }
#endif
            ImGui::EndPopup();
        }
        if (ImGui::BeginPopup("##rename"))
        {
            if (ImGui::IsWindowAppearing())
                ImGui::SetKeyboardFocusHere();
            auto       new_path  = Cool::File::with_extension(Cool::File::without_file_name(project.file_path()) / project._next_name, COOLLAB_FILE_EXTENSION);
            auto const maybe_err = project_name_error_message(project._next_name, new_path, project.file_path());
            if (ImGui::InputText("##name", &project._next_name, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
            {
                if (!maybe_err.has_value())
                {
                    ImGui::CloseCurrentPopup();
                    if (project._next_name != project.name())
                    {
                        auto const old_info_folder = project.info_folder_path();
                        if (Cool::File::rename(project.file_path(), new_path))
                        {
                            project.set_file_path(new_path);
                            Cool::File::rename(old_info_folder, project.info_folder_path());
                            Cool::File::set_content(project.info_folder_path() / "path.txt", Cool::File::weakly_canonical(new_path).string());
#if defined(_WIN32)
                            long_paths_checker().check(new_path);
#endif
                        }
                        else
                        {
                            ImGuiNotify::send({
                                .type    = ImGuiNotify::Type::Warning,
                                .title   = "Failed to rename",
                                .content = "Maybe your new name was too long?",
                            });
                        }
                    }
                }
            }
            if (maybe_err)
                Cool::ImGuiExtras::warning_text(maybe_err->c_str());
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }
    if (project_to_remove != _projects.end())
        _projects.erase(project_to_remove);
    if (project_to_add.has_value())
        _projects.insert(_projects.begin(), *project_to_add);

    if (restored_project.has_value())
    {
        _projects.insert(_projects.begin(), *restored_project);
    }
}