#pragma once
#include <string>
#include <tl/expected.hpp>
#include "CoollabVersion.hpp"
#include "Release.hpp"

class ReleaseManager {
public:
    ReleaseManager();

    [[nodiscard]] auto get_all_release() const -> const std::vector<Release>&;
    [[nodiscard]] auto find(CoollabVersion const& version) const -> Release const*;
    [[nodiscard]] auto latest() const -> Release const*;

    auto no_release_installed() -> bool;

    void imgui();

private:
    std::vector<Release> _releases; // Sorted, from latest to oldest release
};