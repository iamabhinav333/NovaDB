#pragma once

#include <mutex>
#include <string>
#include <string_view>

namespace novadb {

enum class LogLevel {
    Trace = 0,
    Debug,
    Info,
    Warn,
    Error,
};

class Logger {
public:
    static void set_level(LogLevel level);
    static LogLevel level();

    static void trace(std::string_view message);
    static void debug(std::string_view message);
    static void info(std::string_view message);
    static void warn(std::string_view message);
    static void error(std::string_view message);

private:
    static void log(LogLevel message_level, std::string_view message);
    static std::string level_to_string(LogLevel level);

    static std::mutex mutex_;
    static LogLevel level_;
};

}  // namespace novadb
