#pragma once
#include <memory>
#include <chrono>
#include <string>
#include "ITask.h"
#include "CancellationToken.h"

// 简化的调度任务（用于立即执行）
class ScheduledTask {
public:
    using Clock = std::chrono::steady_clock;

    ScheduledTask(std::shared_ptr<ITask> task)
        : task_(std::move(task)) {
    }

    const std::string& Name() const { return name_; }
    std::string Execute(const CancellationTokenPtr& token) {
        return task_->Execute(token);
    }

private:
    std::shared_ptr<ITask> task_;
    std::string name_{ task_ ? task_->GetName() : "Unnamed" };
};