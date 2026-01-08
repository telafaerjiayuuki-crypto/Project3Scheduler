#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <memory>
#include <string>
#include <chrono>

#include "ScheduledTask.h"
#include "LogWriter.h"
#include "ITaskObserver.h"
#include "CancellationToken.h"

class TaskScheduler {
public:
    using Clock = std::chrono::steady_clock;

    static TaskScheduler& Instance();

    void Start(std::shared_ptr<LogWriter> logger);
    void Stop();

    // 立即执行任务（优化后的版本）
    void ExecuteImmediately(std::shared_ptr<ITask> task);

    void AddObserver(std::weak_ptr<ITaskObserver> obs);

    // TaskD 调用：取消当前正在执行的任务
    void CancelCurrent();

private:
    TaskScheduler() = default;
    void WorkerThread();
    void Notify(const TaskEvent& e);

    std::queue<std::shared_ptr<ITask>> taskQueue_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool running_{ false };
    std::thread worker_;

    std::shared_ptr<LogWriter> logger_;

    std::mutex obsMtx_;
    std::vector<std::weak_ptr<ITaskObserver>> observers_;

    std::mutex curMtx_;
    CancellationTokenPtr currentToken_;
};