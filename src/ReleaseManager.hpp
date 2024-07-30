#pragma once
#include <string>
#include <tl/expected.hpp>
#include "Release.hpp"

class ReleaseManager {
public:
    ReleaseManager();

    [[nodiscard]] auto get_all_release() const -> const std::vector<Release>&;
    [[nodiscard]] auto get_latest_release() const -> const Release&;

    auto display_all_release() -> void;
    auto no_release_installed() -> bool;

    auto install_release(const Release& release) -> void;
    auto launch_release(const Release& release) -> void;

private:
    tl::expected<std::vector<Release>, std::string> all_release;
};