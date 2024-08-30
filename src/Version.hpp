#pragma once
#include <compare>
#include <string>

class Version {
public:
    explicit Version(std::string name);

    auto name() const -> std::string const& { return _name; }
    auto is_beta() const -> bool { return _is_beta; }
    auto is_experimental() const -> bool { return _is_experimental; }
    auto major() const -> int { return _major; };
    auto minor() const -> int { return _minor; };
    auto patch() const -> int { return _patch; };

    friend auto operator<=>(Version const&, Version const&) -> std::strong_ordering;

private:
    std::string _name{};
    bool        _is_experimental{false};
    bool        _is_beta{false};
    int         _major{0};
    int         _minor{0};
    int         _patch{0};
};