#include "Task_InstallVersion.hpp"
#include "Cool/File/File.h"
#include "Cool/ImGui/ImGuiExtras.h"
#include "ImGuiNotify/ImGuiNotify.hpp"
#include "Version.hpp"
#include "VersionManager.hpp"
#include "httplib.h"
#include "installation_path.hpp"
#include "miniz.h"
#include "tl/expected.hpp"
#if defined(__APPLE__)
#include <sys/stat.h> // chmod
#endif

static auto download_zip(std::string const& download_url, std::atomic<float>& progression, std::atomic<bool> const& cancel) -> tl::expected<std::string, std::string>
{
    auto cli = httplib::Client{"https://github.com"};
    cli.set_follow_location(true); // Allow the client to follow redirects

    auto res = cli.Get(download_url, [&](uint64_t current, uint64_t total) {
        progression.store(static_cast<float>(current) / static_cast<float>(total));
        return !cancel.load();
    });

    if (res && res->status == 200)
        return res->body;
    return tl::unexpected("Failed to download the file. Status code: " + (res ? std::to_string(res->status) : "Unknown"));
}

static void extract_zip(std::string const& zip, std::filesystem::path const& installation_path, std::atomic<float>& progression)
{
    if (!Cool::File::create_folders_if_they_dont_exist(installation_path))
        return;

    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    if (!mz_zip_reader_init_mem(&zip_archive, zip.data(), zip.size(), 0))
        throw std::runtime_error{fmt::format("Failed to unzip: {}", mz_zip_get_error_string(mz_zip_get_last_error(&zip_archive)))};

    mz_uint num_files = mz_zip_reader_get_num_files(&zip_archive);
    for (mz_uint i = 0; i < num_files; ++i)
    {
        progression.store(static_cast<float>(i) / static_cast<float>(num_files));
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

        std::filesystem::path full_path = installation_path / name;

        if (!Cool::File::create_folders_for_file_if_they_dont_exist(full_path))
            continue;

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
    }
    mz_zip_reader_end(&zip_archive); // TODO(Launcher) RAII
}

static void make_file_executable(std::filesystem::path const& path)
{
#if defined(__linux__)
    std::system(fmt::format("chmod u+x \"{}\"", path.string()).c_str());
#elif defined(__APPLE__)
    if (chmod(path.string().c_str(), 0755) != 0)
        std::cerr << "Failed to set permissions on file: " << full_path << std::endl; // TODO(Launcher) show error to the user, and then uninstall the version. And do the same for the linux implementation?
#else
    std::ignore = path;
#endif
}

Task_InstallVersion::Task_InstallVersion(VersionName name, std::string download_url)
    : _name{std::move(name)}
    , _download_url{std::move(download_url)}
{
    _notification_id = ImGuiNotify::send({
        .type                 = ImGuiNotify::Type::Info,
        .title                = fmt::format("Installing {}", _name.as_string()),
        .custom_imgui_content = [data = _data]() {
            Cool::ImGuiExtras::disabled_if(data->cancel.load(), "", [&]() {
                ImGui::ProgressBar(data->download_progress.load() * 0.95f + 0.05f * data->extraction_progress.load());
                if (ImGui::Button("Cancel"))
                    data->cancel.store(true);
            });
        },
        .duration = std::nullopt,
    });
}

Task_InstallVersion::~Task_InstallVersion()
{
    if (_data->cancel.load())
    {
        version_manager().set_installation_status(_name, InstallationStatus::NotInstalled);
        Cool::File::remove_folder(installation_path(_name)); // Cleanup any files that we might have started to extract from the zip
        ImGuiNotify::close_immediately(_notification_id);
    }
    else
    {
        version_manager().set_installation_status(_name, InstallationStatus::Installed);
        ImGuiNotify::change(
            _notification_id,
            {
                .type    = ImGuiNotify::Type::Success,
                .title   = fmt::format("Installed {}", _name.as_string()),
                .content = "Success",
            }
        );
    }
}

void Task_InstallVersion::do_work()
{
    version_manager().set_installation_status(_name, InstallationStatus::Installing); // TODO(Launcher) should be done in constructor

    auto const zip = download_zip(_download_url, _data->download_progress, _data->cancel);
    if (_data->cancel.load())
        return;
    // TODO(Launcher) handle error zip failed to download
    // TODO(Launcher) handle cancel zip extraction
    extract_zip(*zip, installation_path(_name), _data->extraction_progress);
    if (_data->cancel.load())
        return;
    make_file_executable(executable_path(_name));
}
