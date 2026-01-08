#pragma once
#include "ITask.h"
#include <chrono>
#include <thread>

class SimpleTestTask : public ITask {
public:
    SimpleTestTask(const std::string& name) : name_(name) {}

    std::string GetName() const override {
        return "Test: " + name_;
    }

    std::string Execute(const CancellationTokenPtr& token) override {
        // Ä£Äâ¹¤×÷
        auto start = std::chrono::steady_clock::now();

        for (int i = 0; i < 5; ++i) {
            if (token && token->IsCancelled()) {
                return "Cancelled at step " + std::to_string(i);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        auto end = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        return "Completed in " + std::to_string(ms) + "ms - " + name_;
    }

private:
    std::string name_;
};