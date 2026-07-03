#pragma once

#include <functional>
#include <string>
#include <vector>

#include "novadb/wal/log_record.h"

namespace novadb::wal {

class WriteAheadLog {
public:
    explicit WriteAheadLog(std::string file_path);

    void append(const LogRecord& record);
    std::vector<LogRecord> read_all() const;
    void clear();

    template <typename Callback>
    void replay(Callback&& callback) const {
        for (const auto& record : read_all()) {
            callback(record);
        }
    }

    const std::string& file_path() const;

private:
    std::string file_path_;
};

}  // namespace novadb::wal
