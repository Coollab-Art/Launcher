#include "Release.hpp"
#include <imgui.h>
#include <filesystem>
#include <string>
#include <tl/expected.hpp>
#include "Cool/File/File.h"
#include "Cool/ImGui/ImGuiExtras.h"
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
        auto const notification_id = ImGuiNotify::send({
            .type                 = ImGuiNotify::Type::Info,
            .title                = fmt::format("Installing {}", _release.get_name()),
            .custom_imgui_content = [data = _data]() {
                Cool::ImGuiExtras::disabled_if(data->cancel.load(), "", [&]() {
                    ImGui::TextUnformatted("Downloading");
                    ImGui::ProgressBar(data->download_progress.load());
                    ImGui::TextUnformatted("Extracting");
                    ImGui::ProgressBar(data->extraction_progress.load());
                    if (ImGui::Button("Cancel"))
                        data->cancel.store(true);
                });
            },
            .duration = std::nullopt,
        });

        auto const zip = download_zip(_release, _data->download_progress, _data->cancel);
        if (_data->cancel.load())
        {
            ImGuiNotify::close_immediately(notification_id);
            return;
        }
        // TODO(Launcher) handle error zip failed to download
        // TODO(Launcher) handle cancel zip extraction
        extract_zip(*zip, _release.installation_path(), _data->extraction_progress);
        if (_data->cancel.load())
        {
            ImGuiNotify::close_immediately(notification_id);
            return;
        }
        // make_file_executable(); // TODO(Launcher)
#if defined __linux__
        std::filesystem::path path = _release.executable_path();
        std::system(fmt::format("chmod u+x \"{}\"", path.string()).c_str());
#endif
        // TODO(Launcher) change notification to a Success confirmation?
        ImGuiNotify::close_after_small_delay(notification_id);
    }

    void cancel() override
    {
        _data->cancel.store(true);
    }

    auto needs_user_confirmation_to_cancel_when_closing_app() const -> bool override
    {
        return true;
    }

private:
    Release _release;
    struct DataSharedWithNotification {
        std::atomic<bool>  cancel{false};
        std::atomic<float> download_progress{0.f};
        std::atomic<float> extraction_progress{0.f};
    };
    std::shared_ptr<DataSharedWithNotification> _data{std::make_shared<DataSharedWithNotification>()};
};

void Release::install() const
{
    Cool::task_manager().submit(std::make_shared<Task_InstallRelease>(*this));
}

void Release::uninstall() const
{
    Cool::File::remove_folder(installation_path());
}