#pragma once
#include <iostream>
#include <fstream>
#include <sstream>

// 轻量日志系统：通过 LOG_* 宏输出，可全局调节最低级别，并支持可选的文件日志 sink
// g_logLevel 决定输出哪些级别（>= g_logLevel 才打印）
enum class LogLevel { Debug = 0, Info = 1, Warn = 2, Error = 3, Off = 4 };

inline LogLevel g_logLevel = LogLevel::Info;
inline std::ofstream g_logFile;
inline bool g_logFileOpen = false;

// 打开/切换文件日志（追加模式）。路径为空表示关闭文件日志。
inline void EngineLogSetFile(const std::string& path) {
    if (g_logFileOpen) { g_logFile.close(); g_logFileOpen = false; }
    if (!path.empty()) {
        g_logFile.open(path, std::ios::app);
        g_logFileOpen = g_logFile.is_open();
    }
}
inline void EngineLogClose() { EngineLogSetFile(""); }
inline void EngineLogFlush() { if (g_logFileOpen) g_logFile.flush(); }

// 内部：把 __VA_ARGS__ 串联到流，再一次性写控制台与文件
// 用 lambda + 临时流收集，保持与旧 std::cout << __VA_ARGS__ 等价语义
#define LOG_DEBUG(...) do { if (LogLevel::Debug >= g_logLevel) { std::ostringstream _e_log_ss; _e_log_ss << "[DEBUG] " << __VA_ARGS__ << std::endl; std::cout << _e_log_ss.str(); if (g_logFileOpen) g_logFile << _e_log_ss.str(); } } while(0)
#define LOG_INFO(...)  do { if (LogLevel::Info  >= g_logLevel) { std::ostringstream _e_log_ss; _e_log_ss << "[INFO]  " << __VA_ARGS__ << std::endl; std::cout << _e_log_ss.str(); if (g_logFileOpen) g_logFile << _e_log_ss.str(); } } while(0)
#define LOG_WARN(...)  do { if (LogLevel::Warn  >= g_logLevel) { std::ostringstream _e_log_ss; _e_log_ss << "[WARN]  " << __VA_ARGS__ << std::endl; std::cout << _e_log_ss.str(); if (g_logFileOpen) g_logFile << _e_log_ss.str(); } } while(0)
#define LOG_ERROR(...) do { if (LogLevel::Error >= g_logLevel) { std::ostringstream _e_log_ss; _e_log_ss << "[ERROR] " << __VA_ARGS__ << std::endl; std::cout << _e_log_ss.str(); if (g_logFileOpen) g_logFile << _e_log_ss.str(); } } while(0)