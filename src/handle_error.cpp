#include "handle_error.hpp"
#include <iostream>

void handle_error(std::optional<std::string> const& maybe_error)
{
    if (!maybe_error)
        return;
    handle_error(*maybe_error);
}

void handle_error(std::string const& error)
{
    handle_error(error.c_str());
}

void handle_error(const char* error)
{
    // TODO improve error handling?
    std::cout << "ERROR: " << error << '\n';
}