#include "LogWriter.h"  // 改为相对路径
#include <chrono>
#include <iomanip>
#include <sstream>

static std::string NowStr() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto t = system_clock::to_time_t(now);
    std::tm tm{};
    localtime_s(&tm, &t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

LogWriter::LogWriter(const std::filesystem::path& logPath) {
    std::filesystem::create_directories(logPath.parent_path());
    ofs_.open(logPath, std::ios::out | std::ios::app);
    if (ofs_) {
        ofs_ << "===== Log Open: " << NowStr() << " =====\n";
        ofs_.flush();
    }
}

LogWriter::~LogWriter() {
    if (ofs_) {
        ofs_ << "===== Log Close =====\n";
        ofs_.flush();
        ofs_.close();
    }
}

void LogWriter::WriteLine(const std::string& line) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (!ofs_) return;
    ofs_ << "[" << NowStr() << "] " << line << "\n";
    ofs_.flush();
}