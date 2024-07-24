#include "download.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <vector>
#include "httplib.h"
#include "release.hpp"
#include "utils.hpp"

namespace fs = std::filesystem;

// download file from url
auto download_zip(nlohmann::json const& release) -> tl::expected<std::string, std::string>
{
    std::string     url = "https://github.com";
    httplib::Client cli(url);
    auto const      path = get_coollab_download_url(release);

    cli.set_follow_location(true); // Allow the client to follow redirects

    auto res = cli.Get(path);
    if (res && res->status == 200)
        return res->body;
    return tl::unexpected("Failed to download the file. Status code: " + std::to_string(res->status));
}

auto install_macos_dependencies_if_necessary() -> void
{
    // Check if Homebrew is already installed
    if (std::system("command -v brew >/dev/null 2>&1"))
        install_homebrew();
    else
        std::cout << "Homebrew is already installed." << std::endl;

    // Check if FFmpeg is already installed
    if (std::system("command -v ffmpeg >/dev/null 2>&1"))
        install_ffmpeg();
    else
        std::cout << "FFmpeg is already installed." << std::endl;
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

auto coollab_version_is_installed(std::string_view const& version) -> bool
{
    return fs::exists(get_PATH() / version);
}