#pragma once
#include <Windows.h>
#include "ITaskObserver.h"

constexpr UINT WM_APP_TASK_EVENT = WM_APP + 1;

struct UiEventPayload {
    TaskEvent e;
};

class WinUiObserver : public ITaskObserver {
public:
    explicit WinUiObserver(HWND hwnd) : hwnd_(hwnd) {}
    void OnTaskEvent(const TaskEvent& e) override;

private:
    HWND hwnd_{ nullptr };
};