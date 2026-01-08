#pragma once
#include <filesystem>
#include <string>

bool ZipDirectoryShell(const std::filesystem::path& sourceDir,
    const std::filesystem::path& zipPath,
    std::string* errMsg = nullptr);