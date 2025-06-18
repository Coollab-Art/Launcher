#include "Task_FetchCompatibilityFile.hpp"
#include <sstream>
#include "Cool/File/File.h"
#include "Cool/Task/TaskManager.hpp"
#include "Cool/Utils/getline.hpp"
#include "Path.hpp"
#include "VersionCompatibility.hpp"
#include "make_http_request.hpp"
#include "parse_compatibility_file_line.hpp"

auto Task_FetchCompatibilityFile::execute() -> Cool::TaskCoroutine
{
    auto const res = make_http_request("https://raw.githubusercontent.com/Coollab-Art/Coollab/refs/heads/main/versions_compatibility.txt", [&](uint64_t, uint64_t) {
        return !has_been_canceled();
    });

    if (!res || res->status != 200)
    {
        handle_error(res);
        co_return;
    }

    auto compatibility_entries = std::vector<CompatibilityEntry>{};
    auto string_stream         = std::stringstream{res->body};
    auto line                  = std::string{};
    while (Cool::getline(string_stream, line))
        parse_compatibility_file_line(line, compatibility_entries);
    version_compatibility().set_compatibility_entries(std::move(compatibility_entries));

    Cool::File::set_content(Path::versions_compatibility_file(), res->body);
}

void Task_FetchCompatibilityFile::handle_error(httplib::Result const& res)
{
    Cool::Log::internal_warning(
        "Fetch compatibility file",
        !res ? httplib::to_string(res.error())
             : fmt::format("Status code {}", std::to_string(res->status))
    );

    auto duration_until_reset = std::optional<std::chrono::seconds>{};
    if (res && res->status == 403)
    {
        auto const it = res->headers.find("X-RateLimit-Reset");
        if (it != res->headers.end())
        {
            try
            {
                auto const reset_time_unix   = time_t{std::stoi(it->second)};
                auto const current_time_unix = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                duration_until_reset         = std::chrono::seconds{reset_time_unix - current_time_unix};
            }
            catch (...) // NOLINT(*bugprone-empty-catch)
            {
            }
        }
    }

    if (!res || duration_until_reset.has_value()) // Only retry if we failed because we don't have an Internet connection, or because we hit the max number of requests to Github. There is no point in retrying if the service is unavailable, it's probably not gonna get fixed soon, and if we make too many requests to their API, Github will block us
        Cool::task_manager().submit(after(duration_until_reset.value_or(1s)), std::make_shared<Task_FetchCompatibilityFile>());
}