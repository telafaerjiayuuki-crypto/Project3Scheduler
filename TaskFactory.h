#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Tasks.h"  // 必须包含 Tasks.h，否则不知道具体的任务类

// 前向声明已经不需要了，因为包含了 Tasks.h

// 任务类型枚举
enum class TaskType {
    FileBackup,
    MatrixMultiply,
    HttpGetZen,
    RandomStats
};

// TaskFactory 类 - 用于创建各种任务
class TaskFactory {
public:
    // 主创建方法
    static std::shared_ptr<ITask> CreateTask(TaskType type, const std::vector<std::string>& params = {});

    // 便捷方法（内联实现）
    static inline std::shared_ptr<ITask> CreateFileBackupTask(
        const std::string& src = R"(C:\Data)",  // 使用原始字符串字面量
        const std::string& dstDir = R"(D:\Backup)") {
        return std::make_shared<FileBackupTask>(
            std::filesystem::path(src),
            std::filesystem::path(dstDir)
        );
    }

    static inline std::shared_ptr<ITask> CreateMatrixMultiplyTask(
        const std::string& logDir = "",
        int matrixSize = 100,
        int intervalSeconds = 5) {
        return std::make_shared<MatrixMultiplyContinuousTask>(
            std::filesystem::path(logDir),
            matrixSize,
            intervalSeconds
        );
    }

    static inline std::shared_ptr<ITask> CreateHttpGetTask(
        const std::string& outFile = "zen_output.txt") {
        return std::make_shared<HttpGetZenTask>(
            std::filesystem::path(outFile)
        );
    }

    static inline std::shared_ptr<ITask> CreateHttpGetZenTask(
        const std::string& outFile = "zen.txt") {
        return std::make_shared<HttpGetZenTask>(
            std::filesystem::path(outFile)
        );
    }

    static inline std::shared_ptr<ITask> CreateRandomStatsTask() {
        return std::make_shared<RandomStatsTask>();
    }
};