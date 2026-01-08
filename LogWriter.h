#pragma once
#include <fstream>
#include <mutex>
#include <filesystem>
#include <string>

class LogWriter {
public:
    explicit LogWriter(const std::filesystem::path& logPath);
    ~LogWriter();

    void WriteLine(const std::string& line);

private:
    std::ofstream ofs_;
    std::mutex mtx_;
};