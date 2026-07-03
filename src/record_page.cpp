#include "novadb/storage/record_page.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

namespace novadb::storage {

namespace {

std::uint16_t read_u16(const std::vector<std::uint8_t>& data, std::size_t offset) {
    return static_cast<std::uint16_t>(data[offset] | (static_cast<std::uint16_t>(data[offset + 1]) << 8));
}

void write_u16(std::vector<std::uint8_t>& data, std::size_t offset, std::uint16_t value) {
    data[offset] = static_cast<std::uint8_t>(value & 0xFFU);
    data[offset + 1] = static_cast<std::uint8_t>((value >> 8) & 0xFFU);
}

}  // namespace

RecordPage::RecordPage(buffer::Page& page) : page_(page) { initialize_if_needed(); }

std::optional<std::uint16_t> RecordPage::insert(const Record& record) {
    auto payload = serialize_record(record);
    auto reusable_slot = find_reusable_slot();
    const bool needs_new_slot = !reusable_slot.has_value();

    if (!ensure_free_space(payload.size(), needs_new_slot)) {
        return std::nullopt;
    }

    auto& data = page_.mutable_data();
    const std::uint16_t old_free_end = free_end();
    const std::uint16_t new_offset = static_cast<std::uint16_t>(old_free_end - payload.size());

    std::copy(payload.begin(), payload.end(), data.begin() + new_offset);
    set_free_end(new_offset);

    Slot slot{};
    slot.offset = new_offset;
    slot.size = static_cast<std::uint16_t>(payload.size());
    slot.active = true;

    std::uint16_t slot_id = 0;
    if (reusable_slot.has_value()) {
        slot_id = *reusable_slot;
        set_slot(slot_id, slot);
    } else {
        slot_id = slot_count();
        set_slot_count(static_cast<std::uint16_t>(slot_count() + 1));
        set_slot(slot_id, slot);
        set_free_start(static_cast<std::uint16_t>(kHeaderSize + slot_count() * kSlotSize));
    }

    return slot_id;
}

bool RecordPage::erase(std::uint16_t slot_id) {
    if (slot_id >= slot_count()) {
        return false;
    }

    Slot slot = get_slot(slot_id);
    if (!slot.active) {
        return false;
    }

    slot.active = false;
    set_slot(slot_id, slot);
    page_.mark_dirty(true);
    return true;
}

bool RecordPage::update(std::uint16_t slot_id, const Record& record) {
    if (slot_id >= slot_count()) {
        return false;
    }

    Slot slot = get_slot(slot_id);
    if (!slot.active) {
        return false;
    }

    auto payload = serialize_record(record);

    if (payload.size() <= slot.size) {
        auto& data = page_.mutable_data();
        std::copy(payload.begin(), payload.end(), data.begin() + slot.offset);
        slot.size = static_cast<std::uint16_t>(payload.size());
        set_slot(slot_id, slot);
        return true;
    }

    if (!ensure_free_space(payload.size(), false)) {
        return false;
    }

    auto& data = page_.mutable_data();
    const std::uint16_t old_free_end = free_end();
    const std::uint16_t new_offset = static_cast<std::uint16_t>(old_free_end - payload.size());

    std::copy(payload.begin(), payload.end(), data.begin() + new_offset);
    set_free_end(new_offset);

    slot.offset = new_offset;
    slot.size = static_cast<std::uint16_t>(payload.size());
    slot.active = true;
    set_slot(slot_id, slot);

    return true;
}

std::optional<Record> RecordPage::read(std::uint16_t slot_id) const {
    if (slot_id >= slot_count()) {
        return std::nullopt;
    }

    const Slot slot = get_slot(slot_id);
    if (!slot.active) {
        return std::nullopt;
    }

    const auto& data = page_.data();
    std::vector<std::uint8_t> payload(slot.size, 0);
    std::copy(data.begin() + slot.offset, data.begin() + slot.offset + slot.size, payload.begin());

    return deserialize_record(payload);
}

std::optional<std::uint16_t> RecordPage::search_by_key(std::uint64_t key) const {
    for (std::uint16_t slot_id = 0; slot_id < slot_count(); ++slot_id) {
        auto record = read(slot_id);
        if (record.has_value() && record->key == key) {
            return slot_id;
        }
    }
    return std::nullopt;
}

std::size_t RecordPage::free_space() const {
    const std::uint16_t start = free_start();
    const std::uint16_t end = free_end();
    return end >= start ? static_cast<std::size_t>(end - start) : 0;
}

std::uint16_t RecordPage::slot_count() const {
    return read_u16(page_.data(), 0);
}

std::uint16_t RecordPage::free_start() const {
    return read_u16(page_.data(), 2);
}

std::uint16_t RecordPage::free_end() const {
    return read_u16(page_.data(), 4);
}

void RecordPage::set_slot_count(std::uint16_t value) {
    auto& data = page_.mutable_data();
    write_u16(data, 0, value);
}

void RecordPage::set_free_start(std::uint16_t value) {
    auto& data = page_.mutable_data();
    write_u16(data, 2, value);
}

void RecordPage::set_free_end(std::uint16_t value) {
    auto& data = page_.mutable_data();
    write_u16(data, 4, value);
}

RecordPage::Slot RecordPage::get_slot(std::uint16_t slot_id) const {
    const auto& data = page_.data();
    const std::size_t offset = kHeaderSize + static_cast<std::size_t>(slot_id) * kSlotSize;

    Slot slot{};
    slot.offset = read_u16(data, offset);
    slot.size = read_u16(data, offset + 2);
    slot.active = data[offset + 4] != 0;
    return slot;
}

void RecordPage::set_slot(std::uint16_t slot_id, const Slot& slot) {
    auto& data = page_.mutable_data();
    const std::size_t offset = kHeaderSize + static_cast<std::size_t>(slot_id) * kSlotSize;

    write_u16(data, offset, slot.offset);
    write_u16(data, offset + 2, slot.size);
    data[offset + 4] = static_cast<std::uint8_t>(slot.active ? 1 : 0);
}

std::optional<std::uint16_t> RecordPage::find_reusable_slot() const {
    for (std::uint16_t slot_id = 0; slot_id < slot_count(); ++slot_id) {
        const Slot slot = get_slot(slot_id);
        if (!slot.active) {
            return slot_id;
        }
    }
    return std::nullopt;
}

bool RecordPage::ensure_free_space(std::size_t bytes_needed, bool needs_new_slot) const {
    const std::size_t slot_overhead = needs_new_slot ? kSlotSize : 0;
    return free_space() >= (bytes_needed + slot_overhead);
}

void RecordPage::initialize_if_needed() {
    const auto& current = page_.data();
    const std::uint16_t existing_free_start = read_u16(current, 2);
    const std::uint16_t existing_free_end = read_u16(current, 4);

    if (existing_free_start != 0 || existing_free_end != 0) {
        return;
    }

    auto& data = page_.mutable_data();

    write_u16(data, 0, 0);
    write_u16(data, 2, static_cast<std::uint16_t>(kHeaderSize));
    write_u16(data, 4, static_cast<std::uint16_t>(data.size()));
    page_.mark_dirty(true);
}

}  // namespace novadb::storage
