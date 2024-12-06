#pragma once
#include <optional>
#include <string>

void handle_error(std::optional<std::string> const& maybe_error);
void handle_error(std::string const& error);
void handle_error(const char* error);