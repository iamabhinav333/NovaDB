#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace novadb::storage {

struct Record {
    std::uint64_t key = 0;
    std::string value;
};

std::vector<std::uint8_t> serialize_record(const Record& record);
Record deserialize_record(const std::vector<std::uint8_t>& bytes);

}  // namespace novadb::storage
