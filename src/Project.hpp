#pragma once
#include <chrono>
#include <filesystem>
#include <reg/reg.hpp>
#include <reg/ser20.hpp>
#include <reg/src/AnyId.hpp>
#include <reg/src/utils.hpp>
#include "Cool/Utils/Cached.h"
#include "CoollabVersion.hpp"
#include "Path.hpp"

class Project {
public:
    Project() = default;
    explicit Project(std::filesystem::path file_path, reg::AnyId const& uuid)
        : _file_path{std::move(file_path)}
        , _uuid{uuid}
    {
        init();
    }

    auto file_path() const -> std::filesystem::path const& { return _file_path; }
    auto name() const -> std::string;
    auto id() const -> reg::AnyId const& { return _uuid; }
    auto version() const -> CoollabVersion const&;
    auto thumbnail_path() const -> std::filesystem::path { return info_folder_path() / "thumbnail.png"; }
    auto time_of_last_change() const -> std::filesystem::file_time_type const&;

private:
    auto info_folder_path() const -> std::filesystem::path { return Path::projects_info_folder() / reg::to_string(id()); }
    void init();

private:
    std::filesystem::path                                 _file_path{};
    reg::AnyId                                            _uuid{};
    mutable Cool::Cached<CoollabVersion>                  _coollab_version{};
    mutable Cool::Cached<std::filesystem::file_time_type> _time_of_last_change{};
};