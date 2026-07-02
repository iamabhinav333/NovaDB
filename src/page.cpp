#include "novadb/buffer/page.h"

#include <algorithm>

namespace novadb::buffer {

Page::Page(std::size_t page_id, std::size_t page_size)
    : page_id_(page_id), dirty_(false), pin_count_(0), data_(page_size, 0) {}

std::size_t Page::page_id() const { return page_id_; }

bool Page::is_dirty() const { return dirty_; }

std::size_t Page::pin_count() const { return pin_count_; }

const std::vector<std::uint8_t>& Page::data() const { return data_; }

std::vector<std::uint8_t>& Page::mutable_data() {
    dirty_ = true;
    return data_;
}

void Page::overwrite_data(const std::vector<std::uint8_t>& data) {
    const std::size_t copy_size = std::min(data_.size(), data.size());
    std::copy_n(data.begin(), copy_size, data_.begin());
    if (copy_size < data_.size()) {
        std::fill(data_.begin() + static_cast<std::ptrdiff_t>(copy_size), data_.end(), 0);
    }
    dirty_ = false;
}

void Page::mark_dirty(bool dirty) { dirty_ = dirty; }

void Page::pin() { ++pin_count_; }

void Page::unpin() {
    if (pin_count_ > 0) {
        --pin_count_;
    }
}

}  // namespace novadb::buffer
