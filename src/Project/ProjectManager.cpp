#include "ProjectManager.hpp"
#include <imgui.h>
#include <filesystem>
#include <vector>
#include "Cool/File/File.h"
#include "Cool/ImGui/Fonts.h"
#include "Cool/ImGui/ImGuiExtras.h"
#include "Cool/TextureSource/TextureLibrary_Image.h"
#include "Cool/TextureSource/default_textures.h"
#include "Cool/Utils/overloaded.hpp"
#include "Path.hpp"
#include "Project.hpp"
#include "boxer/boxer.h"
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
    catch (std::exception const&)
    {
        // TODO(Launcher) error
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
        // Items that are outside of the view can be "clipped". We still need to render them because otherwise ImGui doesn't know the
        // size of the whole dropdown list. But we can skip loading the texture, which is the only thing that actually costs some performance anyways.
        bool const is_visible = ImGui::GetWindowPos().y - 1000.f < ImGui::GetCursorScreenPos().y
                                && ImGui::GetCursorScreenPos().y < ImGui::GetWindowPos().y + ImGui::GetWindowSize().y;

        auto& project = *it;
        ImGui::PushID(&project);
        ImGui::PushFont(Cool::Font::bold());
        ImGui::SeparatorText(project.name().c_str());
        ImGui::PopFont();
        if (Cool::ImGuiExtras::big_selectable([&]() {
                Cool::Texture const* thumbnail = is_visible ? Cool::TextureLibrary_Image::instance().get(project.thumbnail_path(), 1s) : &Cool::dummy_texture();
                if (thumbnail)
                {
                    Cool::ImGuiExtras::image_framed(thumbnail->imgui_texture_id(), {100.f, 100.f}, {.frame_thickness = 4.f}); // TODO(Launcher) render alpha checkerboard background bellow it
                    ImGui::SameLine();
                }
                ImGui::BeginGroup();
                ImGui::TextUnformatted(project.file_path().string().c_str());
                // ImGui::PushFont(Cool::Font::italic());
                if (project.current_version().has_value())
                    ImGui::TextUnformatted(project.current_version()->as_string().c_str());
                // ImGui::TextUnformatted(version_manager().label_with_installation_icon(*project.version_name()).c_str());
                else
                    ImGui::TextUnformatted("Unknown version");
                // ImGui::PopFont();
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
                ImGui::EndGroup();
            }))
        {
            launch_project(project);
        }
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::Selectable("Delete project"))
            {
                if (boxer::Selection::OK == boxer::show("Are you sure? This cannot be undone", fmt::format("Deleting project \"{}\"", project.name()).c_str(), boxer::Style::Warning, boxer::Buttons::OKCancel))
                {
                    // TODO(Launcher) move to trash
                    // and move to our own "trash", so that we can CTRL+Z the deletion
                    Cool::File::remove_folder(project.info_folder_path());
                    Cool::File::remove_file(project.file_path());
                    project_to_remove = it;
                }
            }

            project.imgui_version_to_upgrade_to();

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