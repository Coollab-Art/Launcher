#pragma once
#include "VersionName.hpp"

struct Incompatibility {};
struct SemiIncompatibility {
    std::string upgrade_instruction;
};
using CompatibilityEntry = std::variant<VersionName, SemiIncompatibility, Incompatibility>;

void parse_compatibility_file_line(std::string const& line, std::list<CompatibilityEntry>& entries);