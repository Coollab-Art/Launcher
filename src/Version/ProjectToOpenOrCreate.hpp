#pragma once

struct FileToOpen {
    std::filesystem::path path;
};
struct FolderToCreateNewProject {
    std::filesystem::path path;
};
using ProjectToOpenOrCreate = std::variant<FileToOpen, FolderToCreateNewProject>;