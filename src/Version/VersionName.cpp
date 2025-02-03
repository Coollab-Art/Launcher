#include "VersionName.hpp"
#include <cassert>
#include <compare>

static auto is_number(char c) -> bool
{
    return '0' <= c && c <= '9';
}

auto VersionName::from(std::string name) -> std::optional<VersionName>
{
    if (name.empty())
        return std::nullopt;

    VersionName ver;
    ver._name            = std::move(name);
    ver._is_experimental = ver._name.find("Experimental(") != std::string::npos;

    auto       acc                           = std::string{};
    auto       nb_dots                       = 0;
    auto const register_current_version_part = [&]() {
        try
        {
            int const nb = std::stoi(acc);
            acc          = "";
            if (nb_dots == 0)
                ver._major = nb;
            else if (nb_dots == 1)
                ver._minor = nb;
            else
            {
                assert(nb_dots == 2);
                ver._patch = nb;
            }
        }
        catch (...)
        {
            return std::nullopt;
        }
    };

    for (size_t i = 0; i <= ver._name.size(); ++i)
    {
        if (i == ver._name.size())
        {
            register_current_version_part();
            break;
        }

        if (is_number(ver._name[i]))
        {
            acc += ver._name[i];
        }
        else if (ver._name[i] == '.')
        {
            register_current_version_part();
            nb_dots++;
            if (nb_dots > 2)
                break;
        }
        else if (ver._name[i] == ' ')
        {
            register_current_version_part();
            break;
        }
        else
        {
            return std::nullopt;
        }
    }

    return ver;
}

auto operator<=>(VersionName const& a, VersionName const& b) -> std::strong_ordering
{
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

auto operator==(VersionName const& a, VersionName const& b) -> bool
{
    return a._name == b._name;
}

#if defined(COOLLAB_LAUNCHER_TESTS)
#include "doctest/doctest.h"
TEST_CASE("Parsing Coollab Version from string")
{
    SUBCASE("")
    {
        auto const version = VersionName::from("3.7.2 Launcher");
        CHECK(version.has_value());
        CHECK(version->is_experimental() == false);
        CHECK(version->major() == 3);
        CHECK(version->minor() == 7);
        CHECK(version->patch() == 2);
    }
    SUBCASE("")
    {
        auto const version = VersionName::from("18");
        CHECK(version.has_value());
        CHECK(version->is_experimental() == false);
        CHECK(version->major() == 18);
        CHECK(version->minor() == 0);
        CHECK(version->patch() == 0);
    }
    SUBCASE("")
    {
        auto const version = VersionName::from("21.1");
        CHECK(version.has_value());
        CHECK(version->is_experimental() == false);
        CHECK(version->major() == 21);
        CHECK(version->minor() == 1);
        CHECK(version->patch() == 0);
    }
    SUBCASE("")
    {
        auto const version = VersionName::from("5.71.3");
        CHECK(version.has_value());
        CHECK(version->is_experimental() == false);
        CHECK(version->major() == 5);
        CHECK(version->minor() == 71);
        CHECK(version->patch() == 3);
    }
    SUBCASE("")
    {
        auto const version = VersionName::from("5.71.3 Experimental(LED)");
        CHECK(version.has_value());
        CHECK(version->is_experimental() == true);
        CHECK(version->major() == 5);
        CHECK(version->minor() == 71);
        CHECK(version->patch() == 3);
    }
}
#endif