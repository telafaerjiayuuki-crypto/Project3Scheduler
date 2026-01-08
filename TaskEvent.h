#pragma once
#include <string>

enum class TaskEventType { Started, Succeeded, Failed, Cancelled };

struct TaskEvent {
    TaskEventType type;
    std::string taskName;
    std::string message;
};