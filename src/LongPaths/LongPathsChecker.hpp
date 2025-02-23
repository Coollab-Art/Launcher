#pragma once
#include <filesystem>

#if defined(_WIN32)

class LongPathsChecker {
public:
    void check(std::filesystem::path const& path);

private:
    bool _has_sent_notification{false};
};

inline auto long_paths_checker() -> LongPathsChecker&
{
    static auto instance = LongPathsChecker{};
    return instance;
}

#endif