#pragma once
#include <string>
#include <tl/expected.hpp>
#include "release.hpp"

class ReleaseManager {
public:
    ReleaseManager();
    auto               display_all_release() -> void;
    [[nodiscard]] auto               get_all_release() const -> tl::expected<std::vector<Release>, std::string>;
    [[nodiscard]] auto get_latest_release() const -> Release;

    // auto install_release(bool const& latest) -> std::optional<std::string>;
    auto latest_release_is_installed() -> bool;

private:
    tl::expected<std::vector<Release>, std::string> all_release;
};