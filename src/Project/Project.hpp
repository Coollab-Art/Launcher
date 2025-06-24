#pragma once
#include "Cool/File/File.h"
#include "Cool/Utils/Cached.h"
#include "Cool/Utils/hash_project_path_for_info_folder.hpp"
#include "Path.hpp"
#include "Version/VersionName.hpp"
#include "Version/VersionToUpgradeTo.hpp"

class Project {
public:
    Project() = default;
    explicit Project(std::filesystem::path file_path)
        : _file_path{std::move(file_path)}
        , _next_name{Cool::File::file_name_without_extension(_file_path).string()}
    {}

    auto file_path() const -> std::filesystem::path { return Cool::File::weakly_canonical(_file_path); }
    auto file_not_found() const -> bool;
    auto name() const -> std::string;
    auto current_version() const -> std::optional<VersionName>;
    auto version_to_upgrade_to() const -> VersionToUpgradeTo;
    auto version_to_launch() const -> std::optional<VersionName>;
    auto thumbnail_path() const -> std::filesystem::path { return info_folder_path() / "thumbnail.png"; }
    auto time_of_last_change() const -> std::filesystem::file_time_type const&;
    auto info_folder_path() const -> std::filesystem::path { return Path::projects_info_folder() / Cool::hash_project_path_for_info_folder(file_path()); }

    void set_file_path(std::filesystem::path file_path);

    void imgui_version_to_upgrade_to();

    auto category() const -> std::string { return _category; }
    void set_category(std::string const& category) { _category = category; }

private:
    friend class ProjectManager;

    std::filesystem::path                                 _file_path{};
    std::string                                           _next_name{};
    mutable Cool::Cached<std::optional<VersionName>>      _version_name{};
    std::optional<VersionToUpgradeTo>                     _version_to_upgrade_to_selected_by_user{std::nullopt};
    mutable Cool::Cached<std::filesystem::file_time_type> _time_of_last_change{};
    std::string                                           _category{};
};