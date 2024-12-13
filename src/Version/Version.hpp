#pragma once
#include "VersionName.hpp"

class Version {
public:
    Version(VersionName version, std::string download_url)
        : _version(std::move(version)), download_url(std::move(download_url)) {}

    [[nodiscard]] auto get_name() const -> const std::string& { return this->_version.name(); };
    [[nodiscard]] auto get_download_url() const -> const std::string& { return this->download_url; };
    [[nodiscard]] auto version() const -> VersionName const& { return _version; };
    [[nodiscard]] auto installation_path() const -> std::filesystem::path;
    [[nodiscard]] auto executable_path() const -> std::filesystem::path;
    [[nodiscard]] auto is_installed() const -> bool;

    void launch() const;
    void launch(std::filesystem::path const& project_file_path) const;
    void install() const;
    void install_if_necessary() const;
    void uninstall() const;

private:
    VersionName _version;
    std::string download_url;
};