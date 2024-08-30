#include "extractor.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "fmt/core.h"
#include "miniz.h"
#include "utils.hpp"

namespace fs = std::filesystem;

auto extract_zip(std::string const& zip, std::filesystem::path const& installation_path) -> void
{
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    if (!mz_zip_reader_init_mem(&zip_archive, zip.data(), zip.size(), 0))
        throw std::runtime_error{fmt::format("Failed to unzip: {}", mz_zip_get_error_string(mz_zip_get_last_error(&zip_archive)))};

    std::filesystem::path parent_folder = installation_path;
    if (!fs::exists(parent_folder))
        fs::create_directories(parent_folder);

    assert(fs::exists(parent_folder));

    mz_uint num_files = mz_zip_reader_get_num_files(&zip_archive);
    for (mz_uint i = 0; i < num_files; ++i)
    {
        mz_zip_archive_file_stat file_stat;

        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
        {
            std::cerr << "Failed to get file info for index " << i << std::endl;
            continue;
        }

        if (file_stat.m_is_directory)
            continue;

        const char* name = file_stat.m_filename;
        if (name && std::string(name).find("_MACOSX") != std::string::npos)
        {
            continue;
        }

        std::filesystem::path full_path = parent_folder / name;

        fs::create_directories(fs::path(full_path).parent_path());

        std::vector<char> file_data(file_stat.m_uncomp_size);
        if (!mz_zip_reader_extract_to_mem(&zip_archive, i, file_data.data(), file_stat.m_uncomp_size, 0))
        {
            std::cerr << "Failed to read file data: " << name << std::endl;
            continue;
        }

        std::ofstream ofs(full_path, std::ios::binary);
        if (!ofs)
        {
            std::cerr << "Failed to open file for writing: " << full_path << std::endl;
            continue;
        }
        ofs.write(file_data.data(), file_data.size());
        ofs.close();

#ifdef __APPLE__
        if (chmod(full_path.c_str(), 0755) != 0)
            std::cerr << "Failed to set permissions on file: " << full_path << std::endl;
#endif
    }
    mz_zip_reader_end(&zip_archive);
    // std::cout << "✅ Coollab " << version << " is installed! ";
}
