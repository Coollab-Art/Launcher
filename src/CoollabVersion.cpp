#include "CoollabVersion.hpp"
#include <cassert>
#include <compare>

static auto is_number(char c) -> bool
{
    return '0' <= c && c <= '9';
}

CoollabVersion::CoollabVersion(std::string name)
    : _name{std::move(name)}
    , _is_experimental{_name.find("Experimental(") != std::string::npos}
    , _is_beta{_name.starts_with("Beta ")}
{
    auto       acc                           = std::string{};
    auto       nb_dots                       = 0;
    auto const register_current_version_part = [&]() {
        int const nb = std::stoi(acc);
        acc          = "";
        if (nb_dots == 0)
            _major = nb;
        else if (nb_dots == 1)
            _minor = nb;
        else
        {
            assert(nb_dots == 2);
            _patch = nb;
        }
    };

    for (size_t i = _is_beta ? 5 /*skip "Beta "*/ : 0; i <= _name.size(); ++i)
    {
        if (i == _name.size())
        {
            register_current_version_part();
            break;
        }
        if (is_number(_name[i]))
            acc += _name[i];
        else if (_name[i] == '.')
        {
            register_current_version_part();
            nb_dots++;
            if (nb_dots > 2)
                break;
        }
        else
        {
            register_current_version_part();
            break;
        }
    }
}

auto operator<=>(CoollabVersion const& a, CoollabVersion const& b) -> std::strong_ordering
{
    if (a._is_beta && !b._is_beta)
        return std::strong_ordering::less;
    if (!a._is_beta && b._is_beta)
        return std::strong_ordering::greater;

    if (a._major < b._major)
        return std::strong_ordering::less;
    if (b._major < a._major)
        return std::strong_ordering::greater;

    if (a._minor < b._minor)
        return std::strong_ordering::less;
    if (b._minor < a._minor)
        return std::strong_ordering::greater;

    if (a._patch < b._patch)
        return std::strong_ordering::less;
    if (b._patch < a._patch)
        return std::strong_ordering::greater;

    if (a._is_experimental && !b._is_experimental)
        return std::strong_ordering::greater;
    if (!a._is_experimental && b._is_experimental)
        return std::strong_ordering::less;
    if (!a._is_experimental && !b._is_experimental)
        return std::strong_ordering::equal;

    return a._name <=> b._name; // Experimental versions can have the same semantic version, and just differ by their name (eg. "1.2.0 Experimental(LED)" and "1.2.0 Experimental(WebGPU)")
}

#if defined(COOLLAB_LAUNCHER_TESTS)
#include "doctest/doctest.h"
TEST_CASE("Parsing Coollab Version from string")
{
    SUBCASE("")
    {
        auto const version = CoollabVersion{"Beta 18"};
        CHECK(version.is_beta() == true);
        CHECK(version.is_experimental() == false);
        CHECK(version.major() == 18);
        CHECK(version.minor() == 0);
        CHECK(version.patch() == 0);
    }
    SUBCASE("")
    {
        auto const version = CoollabVersion{"Beta 21.1"};
        CHECK(version.is_beta() == true);
        CHECK(version.is_experimental() == false);
        CHECK(version.major() == 21);
        CHECK(version.minor() == 1);
        CHECK(version.patch() == 0);
    }
    SUBCASE("")
    {
        auto const version = CoollabVersion{"Beta 5.71.3"};
        CHECK(version.is_beta() == true);
        CHECK(version.is_experimental() == false);
        CHECK(version.major() == 5);
        CHECK(version.minor() == 71);
        CHECK(version.patch() == 3);
    }
    SUBCASE("")
    {
        auto const version = CoollabVersion{"18"};
        CHECK(version.is_beta() == false);
        CHECK(version.is_experimental() == false);
        CHECK(version.major() == 18);
        CHECK(version.minor() == 0);
        CHECK(version.patch() == 0);
    }
    SUBCASE("")
    {
        auto const version = CoollabVersion{"21.1"};
        CHECK(version.is_beta() == false);
        CHECK(version.is_experimental() == false);
        CHECK(version.major() == 21);
        CHECK(version.minor() == 1);
        CHECK(version.patch() == 0);
    }
    SUBCASE("")
    {
        auto const version = CoollabVersion{"5.71.3"};
        CHECK(version.is_beta() == false);
        CHECK(version.is_experimental() == false);
        CHECK(version.major() == 5);
        CHECK(version.minor() == 71);
        CHECK(version.patch() == 3);
    }
    SUBCASE("")
    {
        auto const version = CoollabVersion{"5.71.3 Experimental(LED)"};
        CHECK(version.is_beta() == false);
        CHECK(version.is_experimental() == true);
        CHECK(version.major() == 5);
        CHECK(version.minor() == 71);
        CHECK(version.patch() == 3);
    }
}
#endif