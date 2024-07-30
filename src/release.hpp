#pragma once
#include <ctime>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
class Release {
public:
    std::string name;
    std::string download_url;
    // need a tag to target the release ?

    [[nodiscard]] auto is_installed() const -> bool;
    auto               operator==(const Release& other) const -> bool;
};

using json = nlohmann::json;
auto get_release(std::string_view const& version) -> tl::expected<nlohmann::basic_json<>, std::string>;
auto get_coollab_download_url(nlohmann::basic_json<> const& release) -> std::string;
// check if Coollab version is already installed
