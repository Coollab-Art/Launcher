#include "parse_compatibility_file_line.hpp"
#include "Cool/String/String.h"

void parse_compatibility_file_line(std::string const& line, std::vector<CompatibilityEntry>& entries)
{
    if (line.starts_with("---"))
    {
        auto const text = Cool::String::substring(line, 3, line.size());
        if (text.empty())
            entries.push_back(Incompatibility{});
        else
            entries.push_back(SemiIncompatibility{text});
    }
    else
    {
        auto const version_name = VersionName::from(line);
        if (!version_name.has_value())
        {
            assert(false);
            return;
        }
        entries.push_back(*version_name);
    }
}