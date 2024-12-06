#pragma once
#include <reg/reg.hpp>
#include <reg/ser20.hpp>
#include "CoollabVersion.hpp"

class Project {
public:
    Project() = default;
    explicit Project(std::filesystem::path file_path)
        : _file_path{std::move(file_path)}
    {
        init();
    }

    auto file_path() const -> std::filesystem::path const& { return _file_path; }
    auto name() const -> std::string const& { return _name; }
    auto version() const -> CoollabVersion const& { return _coollab_version; }

private:
    void init();

private:
    std::filesystem::path _file_path{};
    std::string           _name{};
    reg::AnyId            _uuid{};
    CoollabVersion        _coollab_version{"Unknown"};

private:
    // Serialization
    friend class ser20::access;
    template<class Archive>
    void serialize(Archive& archive)
    {
        archive(
            ser20::make_nvp("File Path", _file_path),
            ser20::make_nvp("ID", _uuid)
        );
    }
};