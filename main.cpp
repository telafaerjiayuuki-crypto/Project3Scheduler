#include <Windows.h>
#include <string>
#include <memory>
#include <filesystem>
#include <thread>
#include <chrono>
#include <atomic>
#include <iostream>

#include "TaskScheduler.h"
#include "TaskFactory.h"
#include "WinUiObserver.h"
#include "LogWriter.h"

// 控件ID
constexpr int IDC_LISTBOX = 1001;
constexpr int IDC_STATIC_RESULT = 1002;
constexpr int IDC_BTN_A = 2001;
constexpr int IDC_BTN_B = 2002;
constexpr int IDC_BTN_C = 2003;
constexpr int IDC_BTN_E = 2004;
constexpr int IDC_BTN_STOP = 2005;
constexpr int IDC_BTN_TEST = 2006;
constexpr int IDC_BTN_TEST_SYSTEM = 2007;  // 测试调度器按钮

constexpr UINT_PTR TIMER_REMINDER = 1;

// 全局控件句柄
static HWND g_listBox = nullptr;
static HWND g_resultText = nullptr;
static std::atomic<bool> g_schedulerRunning = false;

// 辅助函数：向列表框添加文本
static void ListBoxAddLine(const std::wstring& text) {
    if (!g_listBox) return;
    SendMessageW(g_listBox, LB_ADDSTRING, 0, (LPARAM)text.c_str());
    SendMessageW(g_listBox, LB_SETTOPINDEX,
        SendMessageW(g_listBox, LB_GETCOUNT, 0, 0) - 1, 0);

    // 输出到控制台用于调试
    std::wcout << L"[UI] " << text << std::endl;
}

// UTF-8转宽字符
static std::wstring ToWString(const std::string& s) {
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    if (len <= 0) return L"";
    std::wstring ws(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), ws.data(), len);
    return ws;
}

// 设置结果文本
static void SetResultText(const std::wstring& s) {
    if (g_resultText) {
        SetWindowTextW(g_resultText, s.c_str());
        // 强制重绘
        InvalidateRect(g_resultText, nullptr, TRUE);
        UpdateWindow(g_resultText);

        // 输出到控制台用于调试
        std::wcout << L"[Result] " << s << std::endl;
    }
}

// 格式化事件行
static std::wstring FormatEventLine(const TaskEvent& e) {
    std::wstring type;
    switch (e.type) {
    case TaskEventType::Started: type = L"Started"; break;
    case TaskEventType::Succeeded: type = L"Succeeded"; break;
    case TaskEventType::Failed: type = L"Failed"; break;
    case TaskEventType::Cancelled: type = L"Cancelled"; break;
    }

    std::wstring line = L"[";
    line += type;
    line += L"] ";
    line += ToWString(e.taskName);

    if (!e.message.empty()) {
        line += L" - ";
        line += ToWString(e.message);
    }
    return line;
}

// 创建必要目录
static void CreateRequiredDirectories() {
    auto currentPath = std::filesystem::current_path();

    // 创建Data目录
    std::filesystem::path dataDir = currentPath / "Data";
    if (!std::filesystem::exists(dataDir)) {
        std::filesystem::create_directories(dataDir);
        // 创建测试文件
        std::ofstream file1(dataDir / "test1.txt");
        file1 << "Test file 1 for backup - Created at: " << __DATE__ << " " << __TIME__ << "\n";
        file1 << "This is a sample file for TaskA testing.\n";

        std::ofstream file2(dataDir / "test2.txt");
        file2 << "Test file 2 for backup - Created at: " << __DATE__ << " " << __TIME__ << "\n";
        file2 << "This is another sample file for TaskA testing.\n";

        std::cout << "Created Data directory with sample files" << std::endl;
    }

    // 创建Backup目录
    std::filesystem::path backupDir = currentPath / "Backup";
    if (!std::filesystem::exists(backupDir)) {
        std::filesystem::create_directories(backupDir);
        std::cout << "Created Backup directory" << std::endl;
    }

    // 创建logs目录
    std::filesystem::path logsDir = currentPath / "logs";
    if (!std::filesystem::exists(logsDir)) {
        std::filesystem::create_directories(logsDir);
        std::cout << "Created logs directory" << std::endl;
    }
}

// 启动调度器
static void StartScheduler(HWND hwnd) {
    if (g_schedulerRunning) {
        std::cout << "Scheduler already running" << std::endl;
        return;
    }

    CreateRequiredDirectories();

    // 创建日志目录
    std::filesystem::create_directories(std::filesystem::current_path() / "logs");
    auto logger = std::make_shared<LogWriter>(
        std::filesystem::current_path() / "logs" / "scheduler.log");

    // 添加UI观察者
    auto uiObs = std::make_shared<WinUiObserver>(hwnd);
    TaskScheduler::Instance().AddObserver(uiObs);

    // 启动调度器
    TaskScheduler::Instance().Start(logger);
    g_schedulerRunning = true;

    ListBoxAddLine(L"====== Scheduler Started ======");
    ListBoxAddLine(L"Log: " + (std::filesystem::current_path() / "logs" / "scheduler.log").wstring());
    ListBoxAddLine(L"TaskA: Backup Data folder to Backup folder");
    ListBoxAddLine(L"TaskB: Matrix multiplication (100x100)");
    ListBoxAddLine(L"TaskC: Get GitHub Zen -> zen.txt");
    ListBoxAddLine(L"TaskE: Generate random stats -> random_stats.txt");
    ListBoxAddLine(L"");

    std::cout << "Scheduler started successfully" << std::endl;
}

// 停止调度器
static void StopScheduler() {
    if (!g_schedulerRunning) {
        std::cout << "Scheduler not running" << std::endl;
        return;
    }

    TaskScheduler::Instance().Stop();
    g_schedulerRunning = false;
    ListBoxAddLine(L"====== Scheduler Stopped ======");
    ListBoxAddLine(L"");

    std::cout << "Scheduler stopped" << std::endl;
}

// 简单的测试任务类
class SimpleTestTask : public ITask {
public:
    SimpleTestTask(const std::string& name) : name_(name) {}

    std::string GetName() const override {
        return "Test: " + name_;
    }

    std::string Execute(const CancellationTokenPtr& token) override {
        std::cout << "SimpleTestTask::Execute: " << name_ << std::endl;

        // 模拟工作
        for (int i = 1; i <= 3; i++) {
            if (token && token->IsCancelled()) {
                std::cout << "SimpleTestTask cancelled: " << name_ << std::endl;
                return "Cancelled at step " + std::to_string(i);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::cout << "SimpleTestTask progress: " << name_ << " - Step " << i << "/3" << std::endl;
        }

        return "Test task '" + name_ + "' completed successfully at " + __TIME__;
    }

private:
    std::string name_;
};

// 窗口过程
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // 输出调试信息
        std::cout << "WM_CREATE: Creating window controls" << std::endl;

        // 获取窗口客户区大小
        RECT rc;
        GetClientRect(hwnd, &rc);
        int width = rc.right - rc.left;
        int btnWidth = (width - 60) / 5; // 5个按钮，留边距

        // 创建列表框（占窗口上半部分）
        g_listBox = CreateWindowW(L"LISTBOX", nullptr,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT | WS_BORDER | LBS_HASSTRINGS,
            10, 10, width - 20, 250,
            hwnd, (HMENU)(INT_PTR)IDC_LISTBOX,
            GetModuleHandleW(nullptr), nullptr);

        // 设置字体
        HFONT hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas");
        if (hFont) {
            SendMessageW(g_listBox, WM_SETFONT, (WPARAM)hFont, TRUE);
        }

        // 创建结果静态文本
        g_resultText = CreateWindowW(L"STATIC", L"Result: Ready - Click a task button to start",
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX | WS_BORDER,
            10, 270, width - 20, 40,
            hwnd, (HMENU)(INT_PTR)IDC_STATIC_RESULT,
            GetModuleHandleW(nullptr), nullptr);

        if (hFont) {
            SendMessageW(g_resultText, WM_SETFONT, (WPARAM)hFont, TRUE);
        }

        // 创建按钮
        int btnY = 320;
        int btnHeight = 35;

        // Task A
        CreateWindowW(L"BUTTON", L"Task A\nFile Backup",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_MULTILINE,
            10, btnY, btnWidth, btnHeight,
            hwnd, (HMENU)(INT_PTR)IDC_BTN_A, GetModuleHandleW(nullptr), nullptr);

        // Task B
        CreateWindowW(L"BUTTON", L"Task B\nMatrix Multiply",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_MULTILINE,
            20 + btnWidth, btnY, btnWidth, btnHeight,
            hwnd, (HMENU)(INT_PTR)IDC_BTN_B, GetModuleHandleW(nullptr), nullptr);

        // Task C
        CreateWindowW(L"BUTTON", L"Task C\nHTTP GET Zen",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_MULTILINE,
            30 + btnWidth * 2, btnY, btnWidth, btnHeight,
            hwnd, (HMENU)(INT_PTR)IDC_BTN_C, GetModuleHandleW(nullptr), nullptr);

        // Task E
        CreateWindowW(L"BUTTON", L"Task E\nRandom Stats",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_MULTILINE,
            40 + btnWidth * 3, btnY, btnWidth, btnHeight,
            hwnd, (HMENU)(INT_PTR)IDC_BTN_E, GetModuleHandleW(nullptr), nullptr);

        // 测试系统按钮
        CreateWindowW(L"BUTTON", L"Test\nScheduler",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_MULTILINE,
            50 + btnWidth * 4, btnY, btnWidth, btnHeight,
            hwnd, (HMENU)(INT_PTR)IDC_BTN_TEST_SYSTEM, GetModuleHandleW(nullptr), nullptr);

        // 停止按钮
        CreateWindowW(L"BUTTON", L"Stop Scheduler",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, btnY + btnHeight + 15, width - 20, 35,
            hwnd, (HMENU)(INT_PTR)IDC_BTN_STOP, GetModuleHandleW(nullptr), nullptr);

        // 启动调度器
        StartScheduler(hwnd);

        // TaskD：每60秒弹出休息提醒
        SetTimer(hwnd, TIMER_REMINDER, 60000, nullptr); // 60秒 = 60000毫秒
        ListBoxAddLine(L"[TaskD] Rest reminder enabled: every 60 seconds");

        std::cout << "Window creation completed" << std::endl;
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);

        std::cout << "WM_COMMAND: Button " << id << " clicked" << std::endl;

        if (id >= IDC_BTN_A && id <= IDC_BTN_TEST_SYSTEM) {
            if (!g_schedulerRunning) {
                MessageBoxW(hwnd, L"Scheduler is not running. Please restart the program.",
                    L"Error", MB_OK | MB_ICONERROR);
                std::cout << "Scheduler not running, ignoring button click" << std::endl;
                return 0;
            }
        }

        switch (id) {
        case IDC_BTN_A: {  // TaskA - 添加大括号
            std::cout << "Task A button clicked" << std::endl;
            TaskScheduler::Instance().ExecuteImmediately(TaskFactory::CreateFileBackupTask());
            ListBoxAddLine(L"[UI] Task A (File Backup) started");
            break;
        }

        case IDC_BTN_B: {  // TaskB - 添加大括号
            std::cout << "Task B button clicked" << std::endl;
            TaskScheduler::Instance().ExecuteImmediately(TaskFactory::CreateMatrixMultiplyTask());
            ListBoxAddLine(L"[UI] Task B (Matrix Multiply) started");
            break;
        }

        case IDC_BTN_C: {  // TaskC - 添加大括号
            std::cout << "Task C button clicked" << std::endl;
            TaskScheduler::Instance().ExecuteImmediately(TaskFactory::CreateHttpGetTask());
            ListBoxAddLine(L"[UI] Task C (HTTP GET Zen) started");
            break;
        }

        case IDC_BTN_E: {  // TaskE - 添加大括号
            std::cout << "Task E button clicked" << std::endl;
            TaskScheduler::Instance().ExecuteImmediately(TaskFactory::CreateRandomStatsTask());
            ListBoxAddLine(L"[UI] Task E (Random Stats) started");
            break;
        }

        case IDC_BTN_TEST_SYSTEM: {  // 测试调度器 - 添加大括号
            std::cout << "Test System button clicked" << std::endl;
            ListBoxAddLine(L"[TEST] Testing task scheduler system...");

            // 创建并执行测试任务
            auto testTask1 = std::make_shared<SimpleTestTask>("Quick Test 1");
            auto testTask2 = std::make_shared<SimpleTestTask>("Quick Test 2");
            auto testTask3 = std::make_shared<SimpleTestTask>("Quick Test 3");

            TaskScheduler::Instance().ExecuteImmediately(testTask1);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            TaskScheduler::Instance().ExecuteImmediately(testTask2);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            TaskScheduler::Instance().ExecuteImmediately(testTask3);

            ListBoxAddLine(L"[TEST] 3 test tasks started");
            break;
        }

        case IDC_BTN_STOP:  // 停止 - 没有变量声明，可以不加大括号
            std::cout << "Stop button clicked" << std::endl;
            StopScheduler();
            break;

        default:
            // 处理其他命令
            break;
        }
        return 0;
    }
    case WM_TIMER: {
        if (wParam == TIMER_REMINDER) {
            std::cout << "TaskD timer triggered" << std::endl;

            // 先取消当前任务
            TaskScheduler::Instance().CancelCurrent();

            // 弹出休息提醒
            int result = MessageBoxW(hwnd,
                L"⏰ Time for a 5-minute break!\n\n"
                L"Current task has been cancelled.\n"
                L"Stand up, stretch, and rest your eyes.\n\n"
                L"Click OK to continue working.",
                L"TaskD - Rest Reminder",
                MB_OKCANCEL | MB_ICONINFORMATION | MB_DEFBUTTON1);

            if (result == IDOK) {
                ListBoxAddLine(L"[TaskD] Reminder acknowledged. Ready for next task.");
                std::cout << "TaskD: User clicked OK" << std::endl;
            }
            else {
                ListBoxAddLine(L"[TaskD] User cancelled reminder.");
                std::cout << "TaskD: User clicked Cancel" << std::endl;
            }
        }
        return 0;
    }

    case WM_APP_TASK_EVENT: {
        std::cout << "WM_APP_TASK_EVENT received" << std::endl;

        auto* payload = reinterpret_cast<UiEventPayload*>(lParam);
        if (!payload) {
            std::cout << "WM_APP_TASK_EVENT: payload is null" << std::endl;
            return 0;
        }

        const auto& e = payload->e;
        std::cout << "Processing task event: " << e.taskName
            << ", Type: " << static_cast<int>(e.type)
            << ", Msg: " << e.message << std::endl;

        // 添加到列表框
        std::wstring eventLine = FormatEventLine(e);
        ListBoxAddLine(eventLine);

        // 更新结果文本
        std::wstring resultText = L"Result: ";
        resultText += ToWString(e.taskName);
        resultText += L" - ";

        switch (e.type) {
        case TaskEventType::Started:
            resultText += L"Started execution";
            break;
        case TaskEventType::Succeeded:
            resultText += L"✅ Success: " + ToWString(e.message);

            // TaskA完成时弹出消息框
            if (e.taskName == "TaskA File Backup") {
                MessageBoxW(hwnd, L"✅ File backup completed successfully!",
                    L"TaskA - Backup Complete", MB_OK | MB_ICONINFORMATION);
            }
            break;
        case TaskEventType::Failed:
            resultText += L"❌ Failed: " + ToWString(e.message);
            break;
        case TaskEventType::Cancelled:
            resultText += L"⏹️ Cancelled: " + ToWString(e.message);
            break;
        }

        SetResultText(resultText);

        delete payload;
        return 0;
    }

    case WM_CLOSE:
        std::cout << "WM_CLOSE: Closing window" << std::endl;
        KillTimer(hwnd, TIMER_REMINDER);
        StopScheduler();
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        std::cout << "WM_DESTROY: Posting quit message" << std::endl;
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// 程序入口点
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    // 开启控制台窗口用于调试
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);

    std::cout << "=== Project 3 Task Scheduler Starting ===" << std::endl;
    std::cout << "Time: " << __DATE__ << " " << __TIME__ << std::endl;

    const wchar_t CLASS_NAME[] = L"Project3SchedulerWindow";

    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClassW(&wc)) {
        std::cout << "ERROR: Window registration failed!" << std::endl;
        MessageBoxW(nullptr, L"Window registration failed!", L"Error", MB_ICONERROR);
        return 0;
    }

    // 创建主窗口
    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"Project 3 - Task Scheduler (All Tasks Immediate)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 600,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hwnd) {
        std::cout << "ERROR: Window creation failed!" << std::endl;
        MessageBoxW(nullptr, L"Window creation failed!", L"Error", MB_ICONERROR);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    std::cout << "Window created successfully, entering message loop..." << std::endl;

    // 消息循环
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    std::cout << "=== Program Exiting ===" << std::endl;
    return 0;
}