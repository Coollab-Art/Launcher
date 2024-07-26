#pragma once
#include <nlohmann/json.hpp>
#include <string_view>
#include <tl/expected.hpp>
#include <vector>

struct Release {
    std::string name;
    std::string download_url;
    bool        is_latest    = false;

    [[nodiscard]] auto is_installed() const -> bool;
    auto install() -> void;
};

using json = nlohmann::json;
auto get_all_release() -> tl::expected<std::vector<Release>, std::string>;
auto get_release_ready_to_install() -> tl::expected<nlohmann::json, std::string>;
auto get_release(std::string_view const& version) -> tl::expected<nlohmann::basic_json<>, std::string>;
auto get_coollab_download_url(nlohmann::basic_json<> const& release) -> std::string;
// check if Coollab version is already installed

