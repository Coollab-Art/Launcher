#include "Release.hpp"
#include <imgui.h>
#include <filesystem>
#include <string>
#include <tl/expected.hpp>
#include "Cool/File/File.h"
#include "Cool/Task/TaskManager.hpp"
#include "Cool/spawn_process.hpp"
#include "ImGuiNotify/ImGuiNotify.hpp"
#include "Path.hpp"
#include "download.hpp"
#include "extract_zip.hpp"
#include "fmt/core.h"
#include "handle_error.hpp"
#include "httplib.h"
#include "nlohmann/json.hpp"

namespace fs = std::filesystem;

auto Release::installation_path() const -> std::filesystem::path
{
    return Path::installed_versions_folder() / get_name();
}

static auto exe_name() -> std::filesystem::path
{
#if defined(_WIN32)
    return "Coollab.exe";
#elif defined(__linux__) || defined(__APPLE__)
    return "Coollab";
#else
#error "Unsupported platform"
#endif
}

auto Release::executable_path() const -> std::filesystem::path
{
    return installation_path() / exe_name();
}

auto Release::is_installed() const -> bool
{
    return fs::exists(installation_path());
}

// get a single release with tag_name
static auto get_release(std::string_view const& version) -> tl::expected<nlohmann::json, std::string>
{
    std::string url = fmt::format("https://api.github.com/repos/CoolLibs/Lab/releases/tags/{}", version);

    httplib::Client cli("https://api.github.com");
    cli.set_follow_location(true);

    auto res = cli.Get(url);

    if (!res || res->status != 200)
    {
        return tl::make_unexpected(fmt::format("Failed to fetch release info: {}", res ? res->status : -1));
    }

    try
    {
        auto jsonResponse = nlohmann::json::parse(res->body);
        if (!jsonResponse.contains("assets") || jsonResponse["assets"].empty())
        {
            return tl::make_unexpected("No assets found in the release.");
        }
        nlohmann::json const& assets = jsonResponse["assets"];
        return assets;
    }
    catch (nlohmann::json::parse_error const& e)
    {
        return tl::make_unexpected(fmt::format("JSON parsing error: {}", e.what()));
    }
    catch (std::exception& e)
    {
        return tl::make_unexpected(fmt::format("{}", e.what()));
    }
}

void Release::launch() const
{
    handle_error(Cool::spawn_process(executable_path()));
}

void Release::launch(std::filesystem::path const& project_file_path) const
{
    handle_error(Cool::spawn_process(executable_path(), {project_file_path.string()}));
}

void Release::install_if_necessary() const
{
    if (is_installed())
        return;

    install();
}

class Task_InstallRelease : public Cool::Task {
public:
    explicit Task_InstallRelease(Release const& release)
        : _release{release}
    {}

    void do_work() override
    {
        auto download_progression   = std::make_shared<std::atomic<float>>(0.f); // Needs to be a shared_ptr because the Notification will need to keep it alive after this task is done
        auto extraction_progression = std::make_shared<std::atomic<float>>(0.f); // Needs to be an atomic because it will be used on several threads (by the Task and by the Notification)

        auto const notification_id = ImGuiNotify::send({
            .type                 = ImGuiNotify::Type::Info,
            .title                = fmt::format("Installing {}", _release.get_name()),
            .custom_imgui_content = [download_progression, extraction_progression]() { // The lambda needs to capture everything by copy
                ImGui::TextUnformatted("Downloading");
                ImGui::ProgressBar(download_progression->load());
                ImGui::TextUnformatted("Extracting");
                ImGui::ProgressBar(extraction_progression->load());
            },
            .duration = std::nullopt,
        });

        auto const zip = download_zip(_release, *download_progression);
        extract_zip(*zip, _release.installation_path(), *extraction_progression);
        // make_file_executable(); // TODO(Launcher)
#if defined __linux__
        std::filesystem::path path = _release.executable_path();
        std::system(fmt::format("chmod u+x \"{}\"", path.string()).c_str());
#endif
        ImGuiNotify::close(notification_id, 1s);
    }

private:
    Release _release;
};

void Release::install() const
{
    Cool::task_manager().submit(std::make_shared<Task_InstallRelease>(*this));
}

void Release::uninstall() const
{
    Cool::File::remove_folder(installation_path());
}