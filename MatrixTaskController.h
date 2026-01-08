#pragma once
#include <memory>
#include <thread>
#include <atomic>
#include "Tasks.h"
#include "CancellationToken.h"

class MatrixTaskController {
public:
    MatrixTaskController()
        : task_(nullptr),
        token_(std::make_shared<CancellationToken>()),
        workerThread_(),
        isTaskRunning_(false) {
    }

    ~MatrixTaskController() {
        StopTask();
    }

    // 启动任务
    bool StartTask(int matrixSize = 100, int intervalSeconds = 5,
        const std::filesystem::path& logDir = "") {
        if (isTaskRunning_) {
            return false;
        }

        task_ = std::make_shared<MatrixMultiplyContinuousTask>(logDir, matrixSize, intervalSeconds);
        token_->Reset(); // 重置取消令牌

        workerThread_ = std::thread([this]() {
            isTaskRunning_ = true;
            std::string result = task_->Execute(token_);
            std::cout << "Matrix Task completed with result:\n" << result << std::endl;
            isTaskRunning_ = false;
            });

        return true;
    }

    // 停止任务
    void StopTask() {
        if (token_) {
            token_->Cancel();
        }

        if (workerThread_.joinable()) {
            workerThread_.join();
        }

        isTaskRunning_ = false;
    }

    // 暂停/恢复（通过停止和重新启动实现）
    void PauseTask() {
        StopTask();
    }

    void ResumeTask(int matrixSize = 100, int intervalSeconds = 5,
        const std::filesystem::path& logDir = "") {
        StartTask(matrixSize, intervalSeconds, logDir);
    }

    // 获取状态
    bool IsTaskRunning() const {
        return isTaskRunning_ && task_ && task_->IsRunning();
    }

    int GetCurrentIteration() const {
        return task_ ? task_->GetCurrentIteration() : 0;
    }

    std::string GetLastResult() const {
        return task_ ? task_->GetLastResult() : "No task running";
    }

    std::string GetTaskStatus() const {
        if (!task_) return "No task created";
        if (!IsTaskRunning()) return "Task not running";
        return "Running - Iteration: " + std::to_string(GetCurrentIteration()) +
            ", Last Result: " + GetLastResult();
    }

private:
    std::shared_ptr<MatrixMultiplyContinuousTask> task_;
    std::shared_ptr<CancellationToken> token_;
    std::thread workerThread_;
    std::atomic<bool> isTaskRunning_;
};