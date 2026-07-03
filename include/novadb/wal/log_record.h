#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace novadb::wal {

enum class LogType {
    Begin,
    Commit,
    Rollback,
    Insert,
    Update,
    Delete,
};

struct LogRecord {
    std::uint64_t transaction_id = 0;
    LogType type = LogType::Begin;
    std::string table;
    std::uint64_t key = 0;
    std::string value;
};

std::string to_string(LogType type);
LogType log_type_from_string(const std::string& text);

}  // namespace novadb::wal
