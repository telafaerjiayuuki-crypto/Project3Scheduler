#include "Tasks.h"
#include <chrono>
#include <thread>
#include <random>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <future>

// 第三方库包含（实际使用时需要安装）
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <cstdlib>
#endif

// 辅助函数：获取当前日期时间
static std::string GetCurrentDateTime() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif

    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buffer);
}

static std::string GetCurrentDate() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif

    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &tm);
    return std::string(buffer);
}

// -------------------- TaskA: 文件备份（压缩包） --------------------
bool FileBackupTask::CreateZipArchive(const std::filesystem::path& src,
    const std::filesystem::path& zipPath,
    const std::string& logFile) {
    // 使用系统命令创建压缩包

#ifdef _WIN32
    // Windows: 使用PowerShell Compress-Archive
    std::string command;

    // 检查是否是目录
    if (std::filesystem::is_directory(src)) {
        // 压缩整个目录
        command = "powershell -Command \"Compress-Archive -Path '" +
            src.string() + "/*' -DestinationPath '" +
            zipPath.string() + "' -Force\"";
    }
    else {
        // 压缩单个文件
        command = "powershell -Command \"Compress-Archive -Path '" +
            src.string() + "' -DestinationPath '" +
            zipPath.string() + "' -Force\"";
    }
#else
    // Linux/macOS: 使用zip命令
    std::string command;
    if (std::filesystem::is_directory(src)) {
        command = "cd \"" + src.parent_path().string() + "\" && " +
            "zip -rq \"" + zipPath.string() + "\" \"" + src.filename().string() + "\"";
    }
    else {
        command = "zip -q \"" + zipPath.string() + "\" \"" + src.string() + "\"";
    }
#endif

    // 执行系统命令
    int result = system(command.c_str());
    return result == 0;
}

std::string FileBackupTask::Execute(const CancellationTokenPtr& token) {
    std::string timestamp = GetCurrentDateTime();
    std::string dateStr = GetCurrentDate();

    std::cout << "[" << timestamp << "] FileBackupTask::Execute 开始执行" << std::endl;
    std::cout << "源目录: " << src_.string() << std::endl;
    std::cout << "目标目录: " << dstDir_.string() << std::endl;

    // 模拟工作并检查取消
    for (int i = 0; i < 5; i++) {
        if (token && token->IsCancelled()) {
            std::string msg = "备份在步骤 " + std::to_string(i) + " 被取消";
            std::cout << "[" << GetCurrentDateTime() << "] " << msg << std::endl;
            return msg;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    try {
        // 检查源目录是否存在
        if (!std::filesystem::exists(src_)) {
            std::string errorMsg = "源目录不存在: " + src_.string();
            std::cout << "[" << GetCurrentDateTime() << "] " << errorMsg << std::endl;

            // 尝试创建示例数据目录
            std::filesystem::create_directories(src_);
            std::ofstream exampleFile(src_ / "readme.txt");
            if (exampleFile) {
                exampleFile << "这是一个自动创建的示例数据目录\n";
                exampleFile << "创建时间: " << GetCurrentDateTime() << "\n";
                exampleFile << "路径: " << src_.string() << "\n";
                exampleFile << "您可以在此目录放置需要备份的文件\n";
                std::cout << "[" << GetCurrentDateTime() << "] 已创建示例数据目录" << std::endl;
            }
        }

        // 确保目标目录存在
        std::filesystem::create_directories(dstDir_);

        // 创建压缩包文件名：backup_YYYYMMDD.zip
        std::string zipName = "backup_" + dateStr + ".zip";
        std::filesystem::path zipPath = dstDir_ / zipName;

        std::cout << "[" << GetCurrentDateTime() << "] 正在创建压缩包: "
            << zipPath.string() << std::endl;

        // 创建压缩包
        if (CreateZipArchive(src_, zipPath, "")) {
            std::string successMsg = "备份创建成功: " + zipPath.string();

            // 检查压缩包大小
            if (std::filesystem::exists(zipPath)) {
                auto fileSize = std::filesystem::file_size(zipPath);
                successMsg += " (大小: " + std::to_string(fileSize) + " 字节)";
                std::cout << "[" << GetCurrentDateTime() << "] 压缩包大小: "
                    << fileSize << " 字节" << std::endl;
            }

            std::cout << "[" << GetCurrentDateTime() << "] " << successMsg << std::endl;
            return successMsg;
        }
        else {
            std::string errorMsg = "创建压缩包失败: " + zipPath.string();
            std::cout << "[" << GetCurrentDateTime() << "] " << errorMsg << std::endl;

            // 备用方案：创建备份信息文件
            std::filesystem::path infoFile = dstDir_ / ("backup_info_" + dateStr + ".txt");
            std::ofstream info(infoFile);
            if (info) {
                info << "备份时间: " << GetCurrentDateTime() << "\n";
                info << "源目录: " << src_.string() << "\n";
                info << "目标目录: " << dstDir_.string() << "\n";
                info << "状态: 压缩失败\n";
                info << "建议: 请检查系统权限和磁盘空间\n";
                errorMsg += "，已创建信息文件: " + infoFile.string();
            }

            return errorMsg;
        }
    }
    catch (const std::exception& e) {
        std::string errorMsg = "备份错误: " + std::string(e.what());
        std::cout << "[" << GetCurrentDateTime() << "] " << errorMsg << std::endl;
        return errorMsg;
    }
}

// -------------------- TaskB: 矩阵乘法（持续运行版本） --------------------
void MatrixMultiplyContinuousTask::LogToFile(const std::string& message, const std::string& timestamp) {
    std::filesystem::path logPath;
    if (!logDir_.empty()) {
        std::filesystem::create_directories(logDir_);
        logPath = logDir_ / "matrix_continuous_log.txt";
    }
    else {
        logPath = "matrix_continuous_log.txt";
    }

    std::ofstream logFile(logPath, std::ios::app);
    if (logFile) {
        logFile << "[" << timestamp << "] " << message << std::endl;
    }
}

std::string MatrixMultiplyContinuousTask::CalculateTrace(const std::vector<double>& matrix, int size) {
    double trace = 0.0;
    for (int i = 0; i < size; ++i) {
        trace += matrix[i * size + i];
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << trace;
    return oss.str();
}

std::string MatrixMultiplyContinuousTask::PerformMatrixCalculation(const std::string& timestamp) {
    const int N = matrixSize_;
    std::vector<double> A(N * N), B(N * N), C(N * N, 0.0);

    // 使用更高质量的随机数生成器
    std::random_device rd;
    std::mt19937_64 rng(rd());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    // 初始化矩阵（并行初始化以提高性能）
    auto startInit = std::chrono::high_resolution_clock::now();

#pragma omp parallel for if(N * N > 1000)
    for (int i = 0; i < N * N; ++i) {
        A[i] = dist(rng);
        B[i] = dist(rng);
    }

    auto endInit = std::chrono::high_resolution_clock::now();
    auto initTime = std::chrono::duration_cast<std::chrono::microseconds>(endInit - startInit);

    // 矩阵乘法
    auto startCalc = std::chrono::high_resolution_clock::now();

    // 简单的矩阵乘法算法（可以优化为分块或Strassen算法）
#pragma omp parallel for if(N > 200)
    for (int i = 0; i < N; ++i) {
        for (int k = 0; k < N; ++k) {
            double aik = A[i * N + k];
            for (int j = 0; j < N; ++j) {
                C[i * N + j] += aik * B[k * N + j];
            }
        }
    }

    auto endCalc = std::chrono::high_resolution_clock::now();
    auto calcTime = std::chrono::duration_cast<std::chrono::milliseconds>(endCalc - startCalc);

    // 计算迹
    std::string traceStr = CalculateTrace(C, N);

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "Iteration " << iteration_.load()
        << ": Matrix " << N << "x" << N
        << " | Init: " << initTime.count() << "μs"
        << " | Calc: " << calcTime.count() << "ms"
        << " | Trace: " << traceStr
        << " | Total: " << (initTime.count() / 1000.0 + calcTime.count()) << "ms";

    return oss.str();
}

void MatrixMultiplyContinuousTask::Stop() {
    isRunning_ = false;
}

std::string MatrixMultiplyContinuousTask::Execute(const CancellationTokenPtr& token) {
    if (isRunning_) {
        return "Task is already running!";
    }

    std::string startTime = GetCurrentDateTime();
    std::cout << "[" << startTime << "] MatrixMultiplyContinuousTask::Execute started" << std::endl;

    LogToFile("========================================", startTime);
    LogToFile("Starting Continuous Matrix Multiply Task", startTime);
    LogToFile("Matrix Size: " + std::to_string(matrixSize_) + "x" + std::to_string(matrixSize_), startTime);
    LogToFile("Interval: " + std::to_string(intervalSeconds_) + " seconds", startTime);
    LogToFile("========================================", startTime);

    isRunning_ = true;
    iteration_ = 0;

    std::ostringstream fullResult;
    fullResult << "Continuous Matrix Multiply Task Results:\n";
    fullResult << "Started at: " << startTime << "\n";
    fullResult << "Matrix Size: " << matrixSize_ << "x" << matrixSize_ << "\n";
    fullResult << "Interval: " << intervalSeconds_ << " seconds\n";
    fullResult << "----------------------------------------\n";

    try {
        while (isRunning_ && (!token || !token->IsCancelled())) {
            iteration_++;

            std::string iterStartTime = GetCurrentDateTime();
            std::cout << "[" << iterStartTime << "] Starting iteration "
                << iteration_.load() << std::endl;

            LogToFile("Starting iteration " + std::to_string(iteration_), iterStartTime);

            // 执行矩阵计算
            auto calcStart = std::chrono::steady_clock::now();
            std::string result = PerformMatrixCalculation(iterStartTime);
            auto calcEnd = std::chrono::steady_clock::now();

            auto calcDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
                calcEnd - calcStart);

            // 更新最后结果
            lastResult_ = result;

            // 输出结果
            std::cout << "[" << GetCurrentDateTime() << "] " << result << std::endl;
            LogToFile(result, GetCurrentDateTime());

            // 添加到完整结果
            fullResult << "Iteration " << iteration_ << ": " << result << "\n";

            // 计算需要等待的时间（确保总间隔为 intervalSeconds_）
            auto totalDuration = calcDuration;
            if (totalDuration < std::chrono::seconds(intervalSeconds_)) {
                auto remaining = std::chrono::seconds(intervalSeconds_) - totalDuration;

                std::string waitMsg = "Waiting " +
                    std::to_string(std::chrono::duration_cast<std::chrono::seconds>(remaining).count()) +
                    " seconds before next iteration...";

                std::cout << "[" << GetCurrentDateTime() << "] " << waitMsg << std::endl;
                LogToFile(waitMsg, GetCurrentDateTime());

                // 在等待期间定期检查取消和停止信号
                int checkInterval = 100; // 毫秒
                int checks = remaining.count() / checkInterval;

                for (int i = 0; i < checks; i++) {
                    if (!isRunning_ || (token && token->IsCancelled())) {
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(checkInterval));
                }
            }
            else {
                std::string warningMsg = "Calculation took longer than interval! (" +
                    std::to_string(calcDuration.count()) + "ms > " +
                    std::to_string(intervalSeconds_ * 1000) + "ms)";

                std::cout << "[" << GetCurrentDateTime() << "] WARNING: " << warningMsg << std::endl;
                LogToFile("WARNING: " + warningMsg, GetCurrentDateTime());

                // 如果计算时间超过间隔，立即开始下一次迭代
                std::cout << "[" << GetCurrentDateTime() << "] Starting next iteration immediately" << std::endl;
            }

            // 每10次迭代输出一次统计信息
            if (iteration_ % 10 == 0) {
                std::string statsMsg = "Statistics: Completed " + std::to_string(iteration_) +
                    " iterations, running for " +
                    std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::steady_clock::now() -
                        std::chrono::steady_clock::time_point(
                            std::chrono::duration_cast<std::chrono::seconds>(
                                std::chrono::system_clock::now().time_since_epoch()
                            )
                        )
                    ).count()) + " seconds";

                std::cout << "[" << GetCurrentDateTime() << "] " << statsMsg << std::endl;
                LogToFile(statsMsg, GetCurrentDateTime());

                // 保存当前结果到独立文件
                std::filesystem::path checkpointPath;
                if (!logDir_.empty()) {
                    checkpointPath = logDir_ / ("checkpoint_iter_" + std::to_string(iteration_) + ".txt");
                }
                else {
                    checkpointPath = "matrix_checkpoint_iter_" + std::to_string(iteration_) + ".txt";
                }

                std::ofstream checkpointFile(checkpointPath);
                if (checkpointFile) {
                    checkpointFile << "Matrix Multiply Task Checkpoint\n";
                    checkpointFile << "Iteration: " << iteration_ << "\n";
                    checkpointFile << "Time: " << GetCurrentDateTime() << "\n";
                    checkpointFile << "Last Result: " << lastResult_ << "\n";
                    checkpointFile << "Total Iterations: " << iteration_ << "\n";
                }
            }

            // 检查停止信号
            if (!isRunning_ || (token && token->IsCancelled())) {
                break;
            }
        }
    }
    catch (const std::exception& e) {
        std::string errorMsg = "Error during matrix calculation: " + std::string(e.what());
        std::cout << "[" << GetCurrentDateTime() << "] ERROR: " << errorMsg << std::endl;
        LogToFile("ERROR: " + errorMsg, GetCurrentDateTime());
        fullResult << "\n[ERROR] " << errorMsg;
    }

    std::string stopTime = GetCurrentDateTime();
    isRunning_ = false;

    std::string completionMsg = "Continuous matrix calculation stopped after " +
        std::to_string(iteration_) + " iterations";

    std::cout << "[" << stopTime << "] " << completionMsg << std::endl;
    LogToFile(completionMsg, stopTime);
    LogToFile("========================================", stopTime);
    LogToFile("Task completed successfully", stopTime);
    LogToFile("========================================", stopTime);

    fullResult << "\n----------------------------------------\n";
    fullResult << "Stopped at: " << stopTime << "\n";
    fullResult << "Total Iterations: " << iteration_ << "\n";
    fullResult << "Last Result: " << lastResult_ << "\n";
    fullResult << "[" << completionMsg << "]";

    return fullResult.str();
}

// 其他任务的实现保持不变...

// -------------------- TaskC: HTTP请求 --------------------
std::string HttpGetZenTask::Execute(const CancellationTokenPtr& token) {
    std::string timestamp = GetCurrentDateTime();
    std::cout << "[" << timestamp << "] HttpGetZenTask::Execute started" << std::endl;

    // 创建日志文件
    std::filesystem::path logPath = outFile_.parent_path() / "http_zen_log.txt";
    std::ofstream logFile(logPath, std::ios::app);

    if (logFile) {
        logFile << "[" << timestamp << "] HTTP Zen request started" << std::endl;
    }

    // 模拟网络延迟并检查取消
    for (int i = 0; i < 3; i++) {
        if (token && token->IsCancelled()) {
            std::string msg = "HTTP request cancelled";
            std::cout << "[" << GetCurrentDateTime() << "] " << msg << std::endl;

            if (logFile) {
                logFile << "[" << GetCurrentDateTime() << "] " << msg << std::endl;
            }
            return msg;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    try {
        // 模拟GitHub Zen API响应
        const char* zenQuotes[] = {
            "Simplicity is prerequisite for reliability.",
            "It's not fully shipped until it's fast.",
            "Practicality beats purity.",
            "Avoid administrative distraction.",
            "Mind your words, they are important.",
            "Non-blocking is better than blocking.",
            "Design for failure.",
            "Half measures are as bad as nothing at all.",
            "Favor focus over features.",
            "Approachable is better than simple."
        };

        std::mt19937 rng(static_cast<unsigned int>(
            std::chrono::system_clock::now().time_since_epoch().count()));
        std::uniform_int_distribution<size_t> dist(0,
            sizeof(zenQuotes) / sizeof(zenQuotes[0]) - 1);

        std::string zenQuote = zenQuotes[dist(rng)];

        // 写入文件
        std::ofstream ofs(outFile_);
        if (ofs) {
            ofs << "=== GitHub Zen ===\n";
            ofs << "Time: " << timestamp << "\n";
            ofs << "Quote: " << zenQuote << "\n";
            ofs << "==================\n";
        }

        std::string successMsg = "Zen quote saved to " + outFile_.string() + ": " + zenQuote;
        std::cout << "[" << GetCurrentDateTime() << "] " << successMsg << std::endl;

        if (logFile) {
            logFile << "[" << GetCurrentDateTime() << "] " << successMsg << std::endl;
        }

        return successMsg;
    }
    catch (const std::exception& e) {
        std::string errorMsg = "HTTP error: " + std::string(e.what());
        std::cout << "[" << GetCurrentDateTime() << "] " << errorMsg << std::endl;

        if (logFile) {
            logFile << "[" << GetCurrentDateTime() << "] " << errorMsg << std::endl;
        }
        return errorMsg;
    }
}

// -------------------- TaskE: 随机统计 --------------------
std::string RandomStatsTask::Execute(const CancellationTokenPtr& token) {
    std::string timestamp = GetCurrentDateTime();
    std::cout << "[" << timestamp << "] RandomStatsTask::Execute started" << std::endl;

    const int N = 500; // 减少数量确保快速完成
    std::vector<int> numbers;
    numbers.reserve(N);

    std::mt19937 rng(static_cast<unsigned int>(
        std::chrono::system_clock::now().time_since_epoch().count()));
    std::uniform_int_distribution<int> dist(0, 100);

    double sum = 0.0;
    for (int i = 0; i < N; ++i) {
        if (token && token->IsCancelled()) {
            std::string msg = "Random stats calculation cancelled";
            std::cout << "[" << GetCurrentDateTime() << "] " << msg << std::endl;
            return msg;
        }

        int num = dist(rng);
        numbers.push_back(num);
        sum += num;

        // 每100个数检查一次取消
        if (i % 100 == 0 && token && token->IsCancelled()) {
            std::string msg = "Random stats calculation cancelled at iteration " +
                std::to_string(i);
            std::cout << "[" << GetCurrentDateTime() << "] " << msg << std::endl;
            return msg;
        }
    }

    double mean = sum / N;

    double variance = 0.0;
    for (int i = 0; i < N; ++i) {
        if (token && token->IsCancelled()) {
            std::string msg = "Random stats calculation cancelled during variance calculation";
            std::cout << "[" << GetCurrentDateTime() << "] " << msg << std::endl;
            return msg;
        }

        double diff = numbers[i] - mean;
        variance += diff * diff;
    }
    variance /= N;

    double stddev = std::sqrt(variance);

    // 写入文件（附加模式）
    std::filesystem::path logPath = "random_stats_log.txt";
    std::ofstream ofs(logPath, std::ios::app);
    if (ofs) {
        ofs << "[" << timestamp << "] === Random Statistics ===" << std::endl;
        ofs << "[" << timestamp << "] Count: " << N << std::endl;
        ofs << std::fixed << std::setprecision(4);
        ofs << "[" << timestamp << "] Mean: " << mean << std::endl;
        ofs << "[" << timestamp << "] Variance: " << variance << std::endl;
        ofs << "[" << timestamp << "] Standard Deviation: " << stddev << std::endl;
        ofs << "[" << timestamp << "] Min: " << *std::min_element(numbers.begin(), numbers.end()) << std::endl;
        ofs << "[" << timestamp << "] Max: " << *std::max_element(numbers.begin(), numbers.end()) << std::endl;
        ofs << "[" << timestamp << "] ==========================" << std::endl;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4);
    oss << "Generated " << N << " random numbers. ";
    oss << "Mean: " << mean << ", Variance: " << variance << ", StdDev: " << stddev;

    std::cout << "[" << GetCurrentDateTime() << "] RandomStatsTask completed: "
        << oss.str() << std::endl;

    return oss.str();
}