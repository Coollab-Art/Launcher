#include "Project.hpp"
#include "Cool/File/File.h"
#include "Cool/ImGui/IcoMoonCodepoints.h"
#include "Cool/ImGui/ImGuiExtras.h"
#include "Cool/ImGui/ImGuiExtras_dropdown.hpp"
#include "Cool/Utils/getline.hpp"
#include "Cool/Utils/overloaded.hpp"
#include "LauncherSettings.hpp"
#include "Version/VersionName.hpp"
#include "VersionCompatibility/VersionCompatibility.hpp"
#include "range/v3/view.hpp"

auto Project::name() const -> std::string
{
    return Cool::File::file_name_without_extension(_file_path).string();
}

auto Project::file_not_found() const -> bool
{
    auto const not_found = !Cool::File::exists(file_path());
    if (not_found)
        _version_name.invalidate_cache();
    return not_found;
}

auto Project::current_version() const -> std::optional<VersionName>
{
    return _version_name.get_value([&]() -> std::optional<VersionName> {
        auto file = std::ifstream{file_path()};
        if (!file.is_open())
            return std::nullopt;
        auto version = ""s;
        Cool::getline(file, version);
        return VersionName::from(version);
    });
}

auto Project::version_to_upgrade_to() const -> VersionToUpgradeTo
{
    if (_version_to_upgrade_to_selected_by_user.has_value())
        return *_version_to_upgrade_to_selected_by_user;

    if (!launcher_settings().automatically_upgrade_projects_to_latest_compatible_version)
        return DontUpgrade{};

    if (!current_version())
        return DontUpgrade{};

    return version_compatibility().version_to_upgrade_to_automatically(*current_version());
}

auto Project::version_to_launch() const -> std::optional<VersionName>
{
    return std::visit(
        Cool::overloaded{
            [](VersionName const& version_name) -> std::optional<VersionName> {
                return version_name;
            },
            [&](DontUpgrade) -> std::optional<VersionName> {
                return current_version();
            },
        },
        version_to_upgrade_to()
    );
}

auto Project::time_of_last_change() const -> std::filesystem::file_time_type const&
{
    return _time_of_last_change.get_value([&]() {
        return Cool::File::last_write_time(info_folder_path() / "path.txt");
    });
}

static auto as_string(VersionToUpgradeTo const& version) -> std::string
{
    return std::visit(
        Cool::overloaded{
            [&](VersionName const& version_name) {
                return version_name.as_string();
            },
            [](DontUpgrade) {
                return "Don't upgrade"s;
            },
        },
        version
    );
}

struct DropdownEntry_VersionToUpgradeTo {
    VersionToUpgradeTo                 this_entry;
    VersionToUpgradeTo const*          current_version_to_upgrade_to;
    std::optional<VersionToUpgradeTo>* version_selected_by_user;
    bool                               has_semi_incompatibilities{false};

    auto is_selected() const -> bool
    {
        return *current_version_to_upgrade_to == this_entry;
    }

    auto get_label() const -> std::string
    {
        return fmt::format("{}{}", has_semi_incompatibilities ? ICOMOON_WARNING "" : "", as_string(this_entry));
    }

    void apply_value()
    {
        *version_selected_by_user = this_entry;
    }
};

void Project::set_file_path(std::filesystem::path file_path)
{
    _file_path = std::move(file_path);
    _next_name = Cool::File::file_name_without_extension(_file_path).string();
    _version_name.invalidate_cache();
    _time_of_last_change.invalidate_cache();
}

void Project::imgui_version_to_upgrade_to()
{
    const char* label = "Upgrade Coollab version";

    if (!current_version().has_value())
    {
        Cool::ImGuiExtras::disabled_if(true, file_not_found() ? "File not found" : "Unknown version", [&]() {
            ImGui::TextUnformatted(label);
        });
        return;
    }

    auto const compatible_versions = version_compatibility().compatible_versions(*current_version());
    if (compatible_versions.empty())
    {
        Cool::ImGuiExtras::disabled_if(true, fmt::format("This project is already using the latest version compatible with {}", current_version()->as_string()).c_str(), [&]() {
            ImGui::TextUnformatted(label);
        });
        return;
    }

    if (ImGui::BeginMenu(label))
    {
        auto       dropdown_entries  = std::vector<DropdownEntry_VersionToUpgradeTo>{};
        auto const ver_to_upgrade_to = version_to_upgrade_to();
        dropdown_entries.push_back(DropdownEntry_VersionToUpgradeTo{DontUpgrade{}, &ver_to_upgrade_to, &_version_to_upgrade_to_selected_by_user, false /*has_semi_incompatibilities*/});
        for (auto const& version : compatible_versions | ranges::views::reverse)
            dropdown_entries.push_back(DropdownEntry_VersionToUpgradeTo{version.name, &ver_to_upgrade_to, &_version_to_upgrade_to_selected_by_user, !version.upgrade_instructions.empty() /*has_semi_incompatibilities*/});
        Cool::ImGuiExtras::dropdown("##Upgrade version", as_string(version_to_upgrade_to()).c_str(), dropdown_entries);

        std::visit(
            Cool::overloaded{
                [&](VersionName const& version_name) {
                    for (auto const& ver : compatible_versions)
                    {
                        if (ver.name == version_name)
                        {
                            if (!ver.upgrade_instructions.empty())
                                ImGui::TextUnformatted("Some things have changed and might break your project:");
                            for (auto const& instruction : ver.upgrade_instructions | ranges::views::reverse)
                                Cool::ImGuiExtras::warning_text(instruction.c_str());
                            break;
                        }
                    }
                },
                [](DontUpgrade) {},
            },
            version_to_upgrade_to()
        );
        ImGui::EndMenu();
    }
}