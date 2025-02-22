#pragma once
#include <filesystem>
#include <reg/reg.hpp>
#include <reg/ser20.hpp>
#include <reg/src/AnyId.hpp>
#include <reg/src/utils.hpp>
#include "Cool/File/File.h"
#include "Cool/Utils/Cached.h"
#include "Path.hpp"
#include "Version/VersionName.hpp"
#include "Version/VersionToUpgradeTo.hpp"

class Project {
public:
    Project() = default;
    explicit Project(std::filesystem::path file_path, reg::AnyId const& uuid)
        : _file_path{std::move(file_path)}
        , _uuid{uuid}
    {
        init();
    }

    auto file_path() const -> std::filesystem::path { return Cool::File::weakly_canonical(_file_path); }
    auto file_path_exists() const -> bool;
    auto name() const -> std::string;
    auto id() const -> reg::AnyId const& { return _uuid; }
    auto current_version() const -> std::optional<VersionName>;
    auto version_to_upgrade_to() const -> VersionToUpgradeTo;
    auto version_to_launch() const -> std::optional<VersionName>;
    auto thumbnail_path() const -> std::filesystem::path { return info_folder_path() / "thumbnail.png"; }
    auto time_of_last_change() const -> std::filesystem::file_time_type const&;
    auto info_folder_path() const -> std::filesystem::path { return Path::projects_info_folder() / reg::to_string(id()); }

    void imgui_version_to_upgrade_to();

private:
    void init();

private:
    std::filesystem::path                                 _file_path{};
    reg::AnyId                                            _uuid{};
    mutable Cool::Cached<std::optional<VersionName>>      _version_name{};
    std::optional<VersionToUpgradeTo>                     _version_to_upgrade_to_selected_by_user{std::nullopt};
    mutable Cool::Cached<std::filesystem::file_time_type> _time_of_last_change{};
};