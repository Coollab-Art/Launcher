#pragma once
#include <compare>
#include <string>

class VersionName {
public:
    static auto from(std::string name) -> std::optional<VersionName>;

    auto as_string_raw() const -> std::string { return _raw_name; }
    /// Adds quotes around the name part, to make it nicer. But this is isn't suitable for a folder name because quotes are not allowed
    auto as_string_pretty() const -> std::string { return _pretty_name; }

    auto major() const -> int { return _major; };
    auto minor() const -> int { return _minor; };
    auto patch() const -> int { return _patch; };
    auto is_experimental() const -> bool { return _is_experimental; }

    friend auto operator<=>(VersionName const&, VersionName const&) -> std::strong_ordering;
    friend auto operator==(VersionName const&, VersionName const&) -> bool;

private:
    std::string _raw_name{};
    std::string _pretty_name{};
    int         _major{0};
    int         _minor{0};
    int         _patch{0};
    bool        _is_experimental{false};
};