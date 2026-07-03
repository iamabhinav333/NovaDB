#include "novadb/storage/record.h"

#include <cstdint>
#include <stdexcept>

namespace novadb::storage {

namespace {

void append_u64(std::vector<std::uint8_t>& out, std::uint64_t value) {
    for (std::size_t i = 0; i < 8; ++i) {
        out.push_back(static_cast<std::uint8_t>((value >> (i * 8)) & 0xFFU));
    }
}

std::uint64_t read_u64(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    if (offset + 8 > bytes.size()) {
        throw std::runtime_error("Record decode error: missing uint64 field");
    }

    std::uint64_t value = 0;
    for (std::size_t i = 0; i < 8; ++i) {
        value |= static_cast<std::uint64_t>(bytes[offset + i]) << (i * 8);
    }
    return value;
}

}  // namespace

std::vector<std::uint8_t> serialize_record(const Record& record) {
    std::vector<std::uint8_t> out;
    out.reserve(16 + record.value.size());

    append_u64(out, record.key);
    append_u64(out, static_cast<std::uint64_t>(record.value.size()));
    out.insert(out.end(), record.value.begin(), record.value.end());

    return out;
}

Record deserialize_record(const std::vector<std::uint8_t>& bytes) {
    if (bytes.size() < 16) {
        throw std::runtime_error("Record decode error: payload too small");
    }

    Record record{};
    record.key = read_u64(bytes, 0);

    const std::uint64_t value_size = read_u64(bytes, 8);
    if (16 + value_size > bytes.size()) {
        throw std::runtime_error("Record decode error: value length out of range");
    }

    record.value.assign(reinterpret_cast<const char*>(bytes.data() + 16),
                        reinterpret_cast<const char*>(bytes.data() + 16 + value_size));
    return record;
}

}  // namespace novadb::storage
