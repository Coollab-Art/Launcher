#include "handle_error.hpp"
#include <iostream>

void handle_error(std::optional<std::string> const& maybe_error)
{
    if (!maybe_error)
        return;
    // TODO improve error handling?
    std::cout << "ERROR: " << *maybe_error << '\n';
}