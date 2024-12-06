#pragma once
#include "CoollabVersion.hpp"

class Release {
public:
    Release(std::string name, std::string download_url)
        : _version(std::move(name)), download_url(std::move(download_url)) {}

    [[nodiscard]] auto get_name() const -> const std::string& { return this->_version.name(); };
    [[nodiscard]] auto get_download_url() const -> const std::string& { return this->download_url; };
    [[nodiscard]] auto version() const -> CoollabVersion const& { return _version; };
    [[nodiscard]] auto installation_path() const -> std::filesystem::path;
    [[nodiscard]] auto executable_path() const -> std::filesystem::path;
    [[nodiscard]] auto is_installed() const -> bool;

    void launch() const;
    void install() const;
    void uninstall() const;

private:
    CoollabVersion _version;
    std::string    download_url;
};