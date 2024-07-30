#pragma once
#include <ctime>
#include <nlohmann/json.hpp>
#include <string>
#include <tl/expected.hpp>
#include <utility>

class Release {
public:
    Release(std::string name, std::string download_url)
        : name(std::move(name)), download_url(std::move(download_url)) {}

    [[nodiscard]] auto get_name() const -> const std::string& { return this->name; };
    [[nodiscard]] auto get_download_url() const -> const std::string& { return this->download_url; };

    [[nodiscard]] auto is_installed() const -> bool;

    auto operator==(const Release& other) const -> bool;

private:
    std::string name;
    std::string download_url;
};