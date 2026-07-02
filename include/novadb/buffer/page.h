#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace novadb::buffer {

class Page {
public:
    Page(std::size_t page_id, std::size_t page_size);

    std::size_t page_id() const;
    bool is_dirty() const;
    std::size_t pin_count() const;

    const std::vector<std::uint8_t>& data() const;
    std::vector<std::uint8_t>& mutable_data();
    void overwrite_data(const std::vector<std::uint8_t>& data);

    void mark_dirty(bool dirty);
    void pin();
    void unpin();

private:
    std::size_t page_id_;
    bool dirty_;
    std::size_t pin_count_;
    std::vector<std::uint8_t> data_;
};

}  // namespace novadb::buffer
