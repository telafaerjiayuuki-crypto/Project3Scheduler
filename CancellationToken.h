#pragma once
#include <atomic>
#include <memory>

class CancellationToken {
public:
    void Cancel() { cancelled_.store(true, std::memory_order_relaxed); }
    bool IsCancelled() const { return cancelled_.load(std::memory_order_relaxed); }
    void Reset() { cancelled_.store(false, std::memory_order_relaxed); }

private:
    std::atomic<bool> cancelled_{ false };
};

using CancellationTokenPtr = std::shared_ptr<CancellationToken>;