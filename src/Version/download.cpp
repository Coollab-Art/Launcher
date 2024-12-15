#include "download.hpp"
#include <cstdlib>
#include <iostream>
#include <tl/expected.hpp>
#include "Version.hpp"

// download file from url

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
