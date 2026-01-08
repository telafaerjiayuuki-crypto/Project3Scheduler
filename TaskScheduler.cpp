#include "TaskScheduler.h"
#include <chrono>
#include <sstream>
#include <iostream>

TaskScheduler& TaskScheduler::Instance() {
    static TaskScheduler inst;
    return inst;
}

void TaskScheduler::Start(std::shared_ptr<LogWriter> logger) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (running_) return;

    logger_ = std::move(logger);
    running_ = true;
    worker_ = std::thread(&TaskScheduler::WorkerThread, this);

    // 调试输出
    if (logger_) {
        logger_->WriteLine("TaskScheduler started");
    }
    std::cout << "TaskScheduler started" << std::endl;
}

void TaskScheduler::Stop() {
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (!running_) return;
        running_ = false;
    }

    cv_.notify_all();

    if (worker_.joinable()) {
        worker_.join();
    }

    // 调试输出
    if (logger_) {
        logger_->WriteLine("TaskScheduler stopped");
    }
    std::cout << "TaskScheduler stopped" << std::endl;
}

void TaskScheduler::ExecuteImmediately(std::shared_ptr<ITask> task) {
    if (!task) {
        if (logger_) {
            logger_->WriteLine("ExecuteImmediately: null task provided");
        }
        return;
    }

    if (!running_) {
        if (logger_) {
            logger_->WriteLine("ExecuteImmediately called but scheduler not running: " + task->GetName());
        }
        std::cout << "ExecuteImmediately: scheduler not running for " << task->GetName() << std::endl;
        return;
    }

    // 调试输出
    if (logger_) {
        logger_->WriteLine("ExecuteImmediately: " + task->GetName());
    }
    std::cout << "ExecuteImmediately: " << task->GetName() << std::endl;

    {
        std::lock_guard<std::mutex> lk(mtx_);
        taskQueue_.push(std::move(task));
    }

    cv_.notify_one();  // 通知工作线程有新任务
}

void TaskScheduler::CancelCurrent() {
    std::lock_guard<std::mutex> lk(curMtx_);
    if (currentToken_) {
        currentToken_->Cancel();
        // 调试输出
        if (logger_) {
            logger_->WriteLine("Cancelling current task");
        }
        std::cout << "Cancelling current task" << std::endl;
    }
}

void TaskScheduler::AddObserver(std::weak_ptr<ITaskObserver> obs) {
    std::lock_guard<std::mutex> lk(obsMtx_);
    observers_.push_back(std::move(obs));
    // 调试输出
    if (logger_) {
        logger_->WriteLine("Observer added");
    }
    std::cout << "Observer added" << std::endl;
}

void TaskScheduler::Notify(const TaskEvent& e) {
    // 调试输出
    std::cout << "Notify: " << e.taskName << " - ";
    switch (e.type) {
    case TaskEventType::Started: std::cout << "Started"; break;
    case TaskEventType::Succeeded: std::cout << "Succeeded"; break;
    case TaskEventType::Failed: std::cout << "Failed"; break;
    case TaskEventType::Cancelled: std::cout << "Cancelled"; break;
    }
    if (!e.message.empty()) {
        std::cout << " - " << e.message;
    }
    std::cout << std::endl;

    // 写日志
    if (logger_) {
        std::ostringstream oss;
        oss << "Notify: Task=" << e.taskName << " Event=";
        switch (e.type) {
        case TaskEventType::Started:   oss << "Started"; break;
        case TaskEventType::Succeeded: oss << "Succeeded"; break;
        case TaskEventType::Failed:    oss << "Failed"; break;
        case TaskEventType::Cancelled: oss << "Cancelled"; break;
        }
        if (!e.message.empty()) oss << " Msg=" << e.message;
        logger_->WriteLine(oss.str());
    }

    // 通知观察者
    std::vector<std::shared_ptr<ITaskObserver>> activeObservers;
    {
        std::lock_guard<std::mutex> lk(obsMtx_);
        for (auto it = observers_.begin(); it != observers_.end();) {
            if (auto sp = it->lock()) {
                activeObservers.push_back(sp);
                ++it;
            }
            else {
                it = observers_.erase(it);
            }
        }
    }

    // 通知所有活跃的观察者
    for (auto& obs : activeObservers) {
        obs->OnTaskEvent(e);
    }
}

void TaskScheduler::WorkerThread() {
    if (logger_) {
        logger_->WriteLine("WorkerThread started");
    }
    std::cout << "WorkerThread started" << std::endl;

    while (true) {
        std::shared_ptr<ITask> task;

        // 获取任务
        {
            std::unique_lock<std::mutex> lk(mtx_);

            // 等待任务或停止信号
            cv_.wait(lk, [&]() {
                return !running_ || !taskQueue_.empty();
                });

            // 检查是否停止
            if (!running_) {
                if (logger_) {
                    logger_->WriteLine("WorkerThread stopping (running_=false)");
                }
                std::cout << "WorkerThread stopping" << std::endl;
                break;
            }

            // 获取任务
            if (!taskQueue_.empty()) {
                task = taskQueue_.front();
                taskQueue_.pop();

                if (logger_) {
                    logger_->WriteLine("WorkerThread got task: " + task->GetName());
                }
                std::cout << "WorkerThread got task: " << task->GetName() << std::endl;
            }
            else {
                continue;
            }
        }

        // 创建取消令牌
        auto token = std::make_shared<CancellationToken>();
        {
            std::lock_guard<std::mutex> lk(curMtx_);
            currentToken_ = token;
        }

        // 通知任务开始
        Notify({ TaskEventType::Started, task->GetName(), "" });

        std::string result;
        bool taskCancelled = false;

        try {
            if (logger_) {
                logger_->WriteLine("Executing task: " + task->GetName());
            }
            std::cout << "Executing task: " << task->GetName() << std::endl;

            // 执行任务
            result = task->Execute(token);

            // 检查是否取消
            if (token && token->IsCancelled()) {
                taskCancelled = true;
                if (logger_) {
                    logger_->WriteLine("Task cancelled during execution: " + task->GetName());
                }
                std::cout << "Task cancelled: " << task->GetName() << std::endl;
            }
            else {
                if (logger_) {
                    logger_->WriteLine("Task succeeded: " + task->GetName() + " Result: " + result);
                }
                std::cout << "Task succeeded: " << task->GetName() << " Result: " << result << std::endl;
            }
        }
        catch (const std::exception& ex) {
            if (token && token->IsCancelled()) {
                taskCancelled = true;
                if (logger_) {
                    logger_->WriteLine("Task cancelled (exception): " + task->GetName() + " Error: " + ex.what());
                }
                std::cout << "Task cancelled with exception: " << task->GetName() << " - " << ex.what() << std::endl;
            }
            else {
                result = ex.what();
                if (logger_) {
                    logger_->WriteLine("Task failed: " + task->GetName() + " Error: " + ex.what());
                }
                std::cout << "Task failed: " << task->GetName() << " - " << ex.what() << std::endl;
            }
        }
        catch (...) {
            result = "Unknown exception";
            if (logger_) {
                logger_->WriteLine("Task unknown error: " + task->GetName());
            }
            std::cout << "Task unknown error: " << task->GetName() << std::endl;
        }

        // 发送完成通知
        if (taskCancelled) {
            Notify({ TaskEventType::Cancelled, task->GetName(), "Cancelled by user or TaskD" });
        }
        else if (!result.empty() && result.find("cancelled") != std::string::npos) {
            Notify({ TaskEventType::Cancelled, task->GetName(), result });
        }
        else if (result.empty() || result.find("error") != std::string::npos || result.find("Error") != std::string::npos) {
            Notify({ TaskEventType::Failed, task->GetName(), result.empty() ? "Unknown error" : result });
        }
        else {
            Notify({ TaskEventType::Succeeded, task->GetName(), result });
        }

        // 清理当前令牌
        {
            std::lock_guard<std::mutex> lk(curMtx_);
            currentToken_.reset();
        }

        if (logger_) {
            logger_->WriteLine("Task completed: " + task->GetName());
        }
        std::cout << "Task completed: " << task->GetName() << std::endl;
    }

    if (logger_) {
        logger_->WriteLine("WorkerThread ended");
    }
    std::cout << "WorkerThread ended" << std::endl;
}