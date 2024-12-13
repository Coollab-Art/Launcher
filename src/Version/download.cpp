#include "download.hpp"
#include <cstdlib>
#include <iostream>
#include "Version.hpp"
#include "httplib.h"

// download file from url
auto download_zip(const Version& version, std::atomic<float>& progression, std::atomic<bool> const& cancel) -> tl::expected<std::string, std::string>
{
    std::string     url = "https://github.com";
    httplib::Client cli(url);
    auto const&     path = version.get_download_url();

    cli.set_follow_location(true); // Allow the client to follow redirects

    // TODO(Launcher) Cancel download if app closes
    // TODO(Launcher) Also, add a cancel button to cancel installation
    auto res = cli.Get(path, [&](uint64_t current, uint64_t total) {
        progression.store(current / (float)total);
        return !cancel.load();
    });
    if (res && res->status == 200)
        return res->body;
    return tl::unexpected("Failed to download the file. Status code: " + (res ? std::to_string(res->status) : "Unknown"));
}

auto install_macos_dependencies_if_necessary() -> void
{
    // Check if Homebrew is already installed
    if (std::system("command -v brew >/dev/null 2>&1"))
        install_homebrew();

    // Check if FFmpeg is already installed
    if (std::system("command -v ffmpeg >/dev/null 2>&1"))
        install_ffmpeg();
}

auto install_homebrew() -> void
{
    std::cout << "Installing Homebrew..." << std::endl;
    int result = std::system(
        "/bin/bash -c \"$(curl -fsSL "
        "https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
    );
    if (result != 0)
    {
        std::cerr << "Homebrew installation failed!" << std::endl;
        exit(result);
    }
    std::cout << "Homebrew successfully installed." << std::endl;
}

auto install_ffmpeg() -> void
{
    std::cout << "Installing FFmpeg via Homebrew..." << std::endl;
    int result = std::system("brew install ffmpeg");
    if (result != 0)
    {
        std::cerr << "FFmpeg installation failed!" << std::endl;
        exit(result);
    }
    std::cout << "FFmpeg successfully installed." << std::endl;
}
