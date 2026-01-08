#pragma once
#include <Windows.h>

class UniqueHandle {
public:
    UniqueHandle() = default;
    explicit UniqueHandle(HANDLE h) : h_(h) {}
    ~UniqueHandle() { reset(); }

    UniqueHandle(const UniqueHandle&) = delete;
    UniqueHandle& operator=(const UniqueHandle&) = delete;

    UniqueHandle(UniqueHandle&& o) noexcept : h_(o.h_) { o.h_ = nullptr; }
    UniqueHandle& operator=(UniqueHandle&& o) noexcept {
        if (this != &o) {
            reset();
            h_ = o.h_;
            o.h_ = nullptr;
        }
        return *this;
    }

    HANDLE get() const { return h_; }
    HANDLE* put() { reset(); return &h_; }
    explicit operator bool() const { return h_ != nullptr; }

    void reset(HANDLE nh = nullptr) {
        if (h_) CloseHandle(h_);
        h_ = nh;
    }

private:
    HANDLE h_{ nullptr };
};