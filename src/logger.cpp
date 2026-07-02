#include "novadb/common/logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace novadb {

std::mutex Logger::mutex_{};
LogLevel Logger::level_ = LogLevel::Info;

void Logger::set_level(LogLevel level) { level_ = level; }

LogLevel Logger::level() { return level_; }

void Logger::trace(std::string_view message) { log(LogLevel::Trace, message); }
void Logger::debug(std::string_view message) { log(LogLevel::Debug, message); }
void Logger::info(std::string_view message) { log(LogLevel::Info, message); }
void Logger::warn(std::string_view message) { log(LogLevel::Warn, message); }
void Logger::error(std::string_view message) { log(LogLevel::Error, message); }

void Logger::log(LogLevel message_level, std::string_view message) {
    if (static_cast<int>(message_level) < static_cast<int>(level_)) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_snapshot{};
#ifdef _WIN32
    localtime_s(&tm_snapshot, &t);
#else
    localtime_r(&t, &tm_snapshot);
#endif

    std::ostringstream timestamp;
    timestamp << std::put_time(&tm_snapshot, "%Y-%m-%d %H:%M:%S");

    std::cout << '[' << timestamp.str() << "] [" << level_to_string(message_level) << "] " << message << '\n';
}

std::string Logger::level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:
            return "TRACE";
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warn:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

}  // namespace novadb
