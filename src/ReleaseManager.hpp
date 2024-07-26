#pragma once
#include <string>
#include <tl/expected.hpp>
#include "release.hpp"

namespace internal {
}

class ReleaseManager {
public:
    ReleaseManager();
    auto display_all_release() -> void;
    auto install_release(bool const& latest) -> std::optional<std::string>;
    auto latest_release_is_installed() -> bool;

private:
private:
    tl::expected<std::vector<Release>, std::string> all_release;
};