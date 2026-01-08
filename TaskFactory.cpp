#include "TaskFactory.h"
#include <memory>
#include <filesystem>
#include <stdexcept>

// 主创建方法实现
std::shared_ptr<ITask> TaskFactory::CreateTask(TaskType type, const std::vector<std::string>& params) {
    try {
        switch (type) {
        case TaskType::FileBackup:
            if (params.size() >= 2) {
                return CreateFileBackupTask(params[0], params[1]);
            }
            else if (params.size() >= 1) {
                return CreateFileBackupTask(params[0], R"(D:\Backup)");
            }
            else {
                return CreateFileBackupTask();  // 使用默认路径 C:\Data -> D:\Backup
            }
            break;

        case TaskType::MatrixMultiply:
            if (params.size() >= 3) {
                return CreateMatrixMultiplyTask(
                    params[0],
                    std::stoi(params[1]),
                    std::stoi(params[2]));
            }
            else if (params.size() >= 2) {
                return CreateMatrixMultiplyTask(
                    params[0],
                    std::stoi(params[1]),
                    5);  // 默认间隔5秒
            }
            else if (params.size() >= 1) {
                return CreateMatrixMultiplyTask(params[0]);
            }
            else {
                return CreateMatrixMultiplyTask();
            }
            break;

        case TaskType::HttpGetZen:
            if (params.size() >= 1) {
                return CreateHttpGetZenTask(params[0]);
            }
            else {
                return CreateHttpGetZenTask();
            }
            break;

        case TaskType::RandomStats:
            return CreateRandomStatsTask();
            break;

        default:
            break;
        }
    }
    catch (const std::exception& e) {
        // 记录错误但不抛出，返回nullptr
        return nullptr;
    }

    return nullptr;
}