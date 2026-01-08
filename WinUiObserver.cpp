#include "WinUiObserver.h"
#include <iostream>

void WinUiObserver::OnTaskEvent(const TaskEvent& e) {
    std::cout << "WinUiObserver::OnTaskEvent - Task: " << e.taskName
        << ", Type: " << static_cast<int>(e.type)
        << ", Message: " << e.message << std::endl;

    if (!hwnd_) {
        std::cout << "WinUiObserver::OnTaskEvent - hwnd_ is null!" << std::endl;
        return;
    }

    if (!IsWindow(hwnd_)) {
        std::cout << "WinUiObserver::OnTaskEvent - hwnd_ is not a valid window!" << std::endl;
        return;
    }

    auto* payload = new UiEventPayload{ e };

    // 发送消息到UI线程
    BOOL result = PostMessageW(hwnd_, WM_APP_TASK_EVENT, 0, reinterpret_cast<LPARAM>(payload));

    if (!result) {
        DWORD error = GetLastError();
        std::cout << "WinUiObserver::OnTaskEvent - PostMessageW failed! Error: " << error << std::endl;
        delete payload;
    }
    else {
        std::cout << "WinUiObserver::OnTaskEvent - Message posted successfully" << std::endl;
    }
}