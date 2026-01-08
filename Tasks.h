#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <atomic>
#include <memory>
#include "ITask.h"
#include "CancellationToken.h"

// TaskA: 文件备份（压缩包）
class FileBackupTask : public ITask {
public:
    FileBackupTask(std::filesystem::path src, std::filesystem::path dstDir)
        : src_(std::move(src)), dstDir_(std::move(dstDir)) {
    }

    std::string GetName() const override { return "TaskA File Backup"; }
    std::string Execute(const CancellationTokenPtr& token) override;

private:
    std::filesystem::path src_;
    std::filesystem::path dstDir_;

    bool CreateZipArchive(const std::filesystem::path& src,
        const std::filesystem::path& zipPath,
        const std::string& logFile);
};

// TaskB: 矩阵乘法（持续运行版本）
class MatrixMultiplyContinuousTask : public ITask {
public:
    explicit MatrixMultiplyContinuousTask(
        const std::filesystem::path& logDir = "",
        int matrixSize = 100,
        int intervalSeconds = 5)
        : logDir_(logDir),
        matrixSize_(matrixSize),
        intervalSeconds_(intervalSeconds),
        iteration_(0),
        isRunning_(false) {
    }

    ~MatrixMultiplyContinuousTask() {
        Stop();
    }

    std::string GetName() const override { return "TaskB Matrix Multiply (Continuous)"; }
    std::string Execute(const CancellationTokenPtr& token) override;

    // 控制方法
    void Stop();
    bool IsRunning() const { return isRunning_; }
    int GetCurrentIteration() const { return iteration_; }
    std::string GetLastResult() const { return lastResult_; }

private:
    std::filesystem::path logDir_;
    int matrixSize_;
    int intervalSeconds_;
    std::atomic<int> iteration_;
    std::atomic<bool> isRunning_;
    std::string lastResult_;

    std::string PerformMatrixCalculation(const std::string& timestamp);
    void LogToFile(const std::string& message, const std::string& timestamp);
    std::string CalculateTrace(const std::vector<double>& matrix, int size);
};
// TaskC: HTTP请求
class HttpGetZenTask : public ITask {
public:
    explicit HttpGetZenTask(std::filesystem::path outFile)
        : outFile_(std::move(outFile)) {
    }

    std::string GetName() const override { return "TaskC HTTP GET Zen"; }
    std::string Execute(const CancellationTokenPtr& token) override;

private:
    std::filesystem::path outFile_;
};

// TaskE: 随机统计
class RandomStatsTask : public ITask {
public:
    std::string GetName() const override { return "TaskE Random Stats"; }
    std::string Execute(const CancellationTokenPtr& token) override;
};