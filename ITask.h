#pragma once
#include <string>
#include "CancellationToken.h"

class ITask {
public:
    virtual ~ITask() = default;
    virtual std::string GetName() const = 0;
    virtual std::string Execute(const CancellationTokenPtr& token) = 0;
};