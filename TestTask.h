#pragma once
#include "ITask.h"
#include <chrono>
#include <thread>

class TestTask : public ITask {
public:
    TestTask(const std::string& name, int duration_ms = 1000)
        : name_(name), duration_ms_(duration_ms) {
    }

    std::string GetName() const override {
        return "TestTask: " + name_;
    }

    std::string Execute(const CancellationTokenPtr& token) override {
        // Ä£Äâ¹¤×÷
        auto start = std::chrono::steady_clock::now();

        for (int i = 0; i < 10; ++i) {
            if (token && token->IsCancelled()) {
                return "Cancelled at step " + std::to_string(i);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms_ / 10));
        }

        auto end = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        return "Completed in " + std::to_string(ms) + "ms";
    }

private:
    std::string name_;
    int duration_ms_;
};