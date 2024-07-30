#pragma once
#include <string>
#include <tl/expected.hpp>
#include "release.hpp"

class ReleaseManager {
public:
    ReleaseManager();
    auto               display_all_release() -> void;
    [[nodiscard]] auto get_all_release() -> const std::vector<Release>&;
    [[nodiscard]] auto get_latest_release() -> const Release&;
    auto               no_release_installed() -> bool;

private:
    tl::expected<std::vector<Release>, std::string> all_release;
};