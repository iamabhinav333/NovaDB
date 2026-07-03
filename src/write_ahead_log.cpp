#include "novadb/wal/write_ahead_log.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace novadb::wal {

namespace {

LogRecord parse_record(const std::string& line) {
    std::istringstream input(line);
    LogRecord record{};
    std::string type_text;
    if (!(input >> record.transaction_id >> type_text >> std::quoted(record.table) >> record.key >> std::quoted(record.value))) {
        throw std::runtime_error("Failed to parse WAL record: " + line);
    }
    record.type = log_type_from_string(type_text);
    return record;
}

}  // namespace

WriteAheadLog::WriteAheadLog(std::string file_path) : file_path_(std::move(file_path)) {}

void WriteAheadLog::append(const LogRecord& record) {
    std::ofstream out(file_path_, std::ios::app);
    if (!out.good()) {
        throw std::runtime_error("Failed to open WAL file for append: " + file_path_);
    }

    out << record.transaction_id << ' ' << to_string(record.type) << ' ' << std::quoted(record.table) << ' '
        << record.key << ' ' << std::quoted(record.value) << '\n';
    out.flush();
    if (!out.good()) {
        throw std::runtime_error("Failed to append WAL record: " + file_path_);
    }
}

std::vector<LogRecord> WriteAheadLog::read_all() const {
    std::ifstream in(file_path_);
    std::vector<LogRecord> records;
    if (!in.good()) {
        return records;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        records.push_back(parse_record(line));
    }
    return records;
}

void WriteAheadLog::clear() {
    std::ofstream out(file_path_, std::ios::trunc);
    if (!out.good()) {
        throw std::runtime_error("Failed to clear WAL file: " + file_path_);
    }
}

const std::string& WriteAheadLog::file_path() const { return file_path_; }

}  // namespace novadb::wal
