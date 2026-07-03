#include "novadb/wal/log_record.h"

#include <stdexcept>

namespace novadb::wal {

std::string to_string(LogType type) {
    switch (type) {
        case LogType::Begin:
            return "BEGIN";
        case LogType::Commit:
            return "COMMIT";
        case LogType::Rollback:
            return "ROLLBACK";
        case LogType::Insert:
            return "INSERT";
        case LogType::Update:
            return "UPDATE";
        case LogType::Delete:
            return "DELETE";
        default:
            throw std::runtime_error("Unknown WAL log type");
    }
}

LogType log_type_from_string(const std::string& text) {
    if (text == "BEGIN") return LogType::Begin;
    if (text == "COMMIT") return LogType::Commit;
    if (text == "ROLLBACK") return LogType::Rollback;
    if (text == "INSERT") return LogType::Insert;
    if (text == "UPDATE") return LogType::Update;
    if (text == "DELETE") return LogType::Delete;
    throw std::runtime_error("Unknown WAL log type string: " + text);
}

}  // namespace novadb::wal
