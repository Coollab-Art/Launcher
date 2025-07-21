#include "make_http_request.hpp"
#include "Cool/String/String.h"

auto make_http_request(std::string_view url, std::function<bool(uint64_t current, uint64_t total)> progress_callback) -> httplib::Result
{
    assert(url.starts_with("https://"));
    auto cli = httplib::Client{std::string{Cool::String::substring(url, 0, url.find('/', "https://"sv.size()))}};

#if defined(__linux__)
    // On some Linux distros httplib doesn't find the ca certificates automatically, so we have to try a few paths manually
    static constexpr auto ca_paths = std::array{
        "/etc/ssl/certs/ca-certificates.crt",                // Debian/Ubuntu
        "/etc/pki/tls/certs/ca-bundle.crt",                  // RHEL/CentOS/Fedora
        "/etc/ssl/ca-bundle.pem",                            // SUSE
        "/etc/pki/tls/cacert.pem",                           // Slackware
        "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem", // RHEL 7+
        "/etc/ssl/cert.pem",                                 // Alpine, macOS (not Linux but often included)
        "/usr/local/share/certs/ca-root-nss.crt",            // FreeBSD (not Linux but often referenced)
        "/etc/openssl/certs/ca-certificates.crt",            // Some custom setups
        "/usr/share/ssl/certs/ca-bundle.crt",                // Legacy Red Hat
        "/etc/pki/ca-trust/source/anchors/ca-bundle.crt",    // System trust source
        "/var/lib/ca-certificates/ca-bundle.pem",            // openSUSE dynamic
        "/etc/ca-certificates/extracted/tls-ca-bundle.pem",  // Less common variant
    };

    for (auto const& path : ca_paths)
    {
        if (Cool::File::exists(path))
        {
            cli.set_ca_cert_path(path);
            break;
        }
    }
#endif

    // If page has been moved but there is a redirection from the old url to the new one, follow it
    cli.set_follow_location(true);
    // Don't cancel if we have a bad internet connection. This is done in a Task so this is non-blocking anyways (NB: setting too big of a timeout (e.g. 99999h) caused the request to immediately fail on MacOS and Arch Linux)
    cli.set_connection_timeout(15min);
    cli.set_read_timeout(15min);
    cli.set_write_timeout(15min);

    auto res = cli.Get(std::string{url}, std::move(progress_callback));

    if (!res)
        Cool::Log::internal_warning("make_http_request", httplib::to_string(res.error()));
    if (res->status != 200)
        Cool::Log::internal_warning("make_http_request", fmt::format("Error {}\n{}", std::to_string(res->status), res->body));

    return res;
}