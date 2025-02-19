#include "make_http_request.hpp"
#include "Cool/String/String.h"

auto make_http_request(std::string_view url, std::function<bool(uint64_t current, uint64_t total)> progress_callback) -> httplib::Result
{
    assert(url.starts_with("https://"));
    auto cli = httplib::Client{std::string{Cool::String::substring(url, 0, url.find('/', "https://"sv.size()))}};

    // If page has been moved but there is a redirection from the old url to the new one, follow it
    cli.set_follow_location(true);
    // Don't cancel if we have a bad internet connection. This is done in a Task so this is non-blocking anyways
    cli.set_connection_timeout(99999h);
    cli.set_read_timeout(99999h);
    cli.set_write_timeout(99999h);

    return cli.Get(std::string{url}, std::move(progress_callback));
}