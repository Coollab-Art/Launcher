#include "ProjectManager.hpp"
#include <imgui.h>
#include <filesystem>
#include <vector>
#include "Cool/File/File.h"
#include "Cool/ImGui/Fonts.h"
#include "Cool/ImGui/ImGuiExtras.h"
#include "Cool/TextureSource/TextureLibrary_Image.h"
#include "Path.hpp"
#include "Project.hpp"
#include "open/open.hpp"

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

            auto const uuid       = Cool::File::file_name(entry.path()).string();
            auto const maybe_uuid = uuids::uuid::from_string(uuid);
            if (!maybe_uuid)
            {
                // throw std::runtime_error{"[load(uuids::uuid)] Couldn't parse uuid: " + value}; // TODO(Launcher) handle error
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
            _projects.emplace_back(path, *maybe_uuid);
        }
    }
    catch (std::exception const& e)
    {
        // return fmt::format("{}", e.what());
    }
    std::sort(_projects.begin(), _projects.end(), [](Project const& a, Project const& b) {
        return a.time_of_last_change() > b.time_of_last_change();
    });
}

void ProjectManager::imgui(std::function<void(Project const&)> const& launch_project)
{
    auto project_to_remove = _projects.end();
    for (auto it = _projects.begin(); it != _projects.end(); ++it)
    {
        auto const& project = *it;
        ImGui::PushID(&project);
        ImGui::PushFont(Cool::Font::bold());
        ImGui::SeparatorText(project.name().c_str());
        ImGui::PopFont();
        if (Cool::ImGuiExtras::big_selectable([&]() {
                Cool::Texture const* thumbnail = Cool::TextureLibrary_Image::instance().get(project.thumbnail_path());
                if (thumbnail)
                {
                    Cool::ImGuiExtras::image_framed(thumbnail->imgui_texture_id(), {100.f, 100.f}, {.frame_thickness = 4.f}); // TODO(Launcher) render alpha checkerboard background bellow it
                    ImGui::SameLine();
                }
                ImGui::BeginGroup();
                ImGui::TextUnformatted(project.file_path().string().c_str());
                ImGui::PushFont(Cool::Font::italic());
                ImGui::TextUnformatted(project.version_name().as_string().c_str());
                ImGui::PopFont();
                ImGui::EndGroup();
            }))
        {
            launch_project(project);
        }
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::Selectable("Delete project"))
            {
                // TODO(Launcher) move to trash
                // and move to our own "trash", so that we can CTRL+Z the deletion
                // TODO(Launcher) boxer confirmation windows
                Cool::File::remove_folder(project.info_folder_path());
                Cool::File::remove_file(project.file_path());
                project_to_remove = it;
            }
            if (ImGui::Selectable("Reveal in File Explorer"))
            {
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
        ImGui::PopID();
    }
    if (project_to_remove != _projects.end())
        _projects.erase(project_to_remove);
}

void ProjectManager::launch(std::filesystem::path const& project_path)
{
    // auto const* release = release_manager.get(project.coollab_version());
    // if (!release)
    // {
    //     boxer::show(
    //         fmt::format("Please connect to the Internet so that we can install Coollab {}.\n This version is required to launch project \"\"", project.coollab_version().name(), project.name()),
    //         "You are offline"
    //     );
    //     return;
    // }
    // release.launch(project.file_path());
}
