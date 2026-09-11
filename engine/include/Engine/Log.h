#pragma once
#include <iostream>

// 轻量日志系统：通过 LOG_* 宏输出，可全局调节最低级别
// g_logLevel 决定输出哪些级别（>= g_logLevel 才打印）
enum class LogLevel { Debug = 0, Info = 1, Warn = 2, Error = 3, Off = 4 };

inline LogLevel g_logLevel = LogLevel::Info;

#define LOG_DEBUG(...) do { if (LogLevel::Debug >= g_logLevel) { std::cout << "[DEBUG] "; std::cout << __VA_ARGS__ << std::endl; } } while(0)
#define LOG_INFO(...)  do { if (LogLevel::Info  >= g_logLevel) { std::cout << "[INFO]  "; std::cout << __VA_ARGS__ << std::endl; } } while(0)
#define LOG_WARN(...)  do { if (LogLevel::Warn  >= g_logLevel) { std::cout << "[WARN]  "; std::cout << __VA_ARGS__ << std::endl; } } while(0)
#define LOG_ERROR(...) do { if (LogLevel::Error >= g_logLevel) { std::cout << "[ERROR] "; std::cout << __VA_ARGS__ << std::endl; } } while(0)
