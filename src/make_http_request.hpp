#pragma once
#include "httplib.h"

auto make_http_request(std::string_view url, std::function<bool(uint64_t current, uint64_t total)> progress_callback) -> httplib::Result;