#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include "novadb/buffer/page.h"
#include "novadb/storage/record.h"

namespace novadb::storage {

class RecordPage {
public:
    explicit RecordPage(buffer::Page& page);

    std::optional<std::uint16_t> insert(const Record& record);
    bool erase(std::uint16_t slot_id);
    bool update(std::uint16_t slot_id, const Record& record);

    std::optional<Record> read(std::uint16_t slot_id) const;
    std::optional<std::uint16_t> search_by_key(std::uint64_t key) const;

    std::size_t free_space() const;

private:
    struct Slot {
        std::uint16_t offset = 0;
        std::uint16_t size = 0;
        bool active = false;
    };

    static constexpr std::size_t kHeaderSize = 6;
    static constexpr std::size_t kSlotSize = 5;

    buffer::Page& page_;

    std::uint16_t slot_count() const;
    std::uint16_t free_start() const;
    std::uint16_t free_end() const;

    void set_slot_count(std::uint16_t value);
    void set_free_start(std::uint16_t value);
    void set_free_end(std::uint16_t value);

    Slot get_slot(std::uint16_t slot_id) const;
    void set_slot(std::uint16_t slot_id, const Slot& slot);

    std::optional<std::uint16_t> find_reusable_slot() const;
    bool ensure_free_space(std::size_t bytes_needed, bool needs_new_slot) const;

    void initialize_if_needed();
};

}  // namespace novadb::storage
