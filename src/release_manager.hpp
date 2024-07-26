#pragma once
#include "release.hpp"
class ReleaseManager {
public:
    ReleaseManager();
    std::string          latest_release_tag;
    std::vector<Release> all_release_available;
    std::vector<Release> all_release_installed;
    auto                 display_all_release_available() -> void;
    auto                 install_release(bool const &latest) -> std::optional<std::string>;

private:
    auto get_latest_release_tag() -> std::string;
    auto get_all_release_available() -> std::optional<std::string>;
    auto get_all_release_installed() -> std::optional<std::string>;
};
