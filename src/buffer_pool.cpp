#include "novadb/buffer/buffer_pool.h"

#include <stdexcept>

namespace novadb::buffer {

BufferPoolManager::BufferPoolManager(IPageIO& page_io, std::size_t page_size, std::size_t pool_size)
    : page_io_(page_io), page_size_(page_size), pool_size_(pool_size) {
    if (pool_size_ == 0) {
        throw std::invalid_argument("Buffer pool size must be greater than zero");
    }
}

std::shared_ptr<Page> BufferPoolManager::fetch_page(std::size_t page_id) {
    if (auto it = frames_.find(page_id); it != frames_.end()) {
        it->second.page->pin();
        touch(page_id);
        return it->second.page;
    }

    evict_if_needed();

    Frame frame{};
    frame.page = std::make_shared<Page>(page_id, page_size_);
    frame.page->overwrite_data(page_io_.read_page(page_id, page_size_));
    frame.page->pin();
    frame.in_lru = false;

    frames_.emplace(page_id, std::move(frame));
    return frames_.at(page_id).page;
}

std::shared_ptr<Page> BufferPoolManager::create_page(std::size_t page_id) {
    if (auto it = frames_.find(page_id); it != frames_.end()) {
        it->second.page->pin();
        touch(page_id);
        return it->second.page;
    }

    evict_if_needed();

    Frame frame{};
    frame.page = std::make_shared<Page>(page_id, page_size_);
    frame.page->pin();
    frame.page->mark_dirty(true);
    frame.in_lru = false;

    frames_.emplace(page_id, std::move(frame));
    return frames_.at(page_id).page;
}

bool BufferPoolManager::unpin_page(std::size_t page_id, bool mark_dirty) {
    auto it = frames_.find(page_id);
    if (it == frames_.end()) {
        return false;
    }

    if (mark_dirty) {
        it->second.page->mark_dirty(true);
    }

    it->second.page->unpin();
    maybe_add_to_lru(page_id);
    return true;
}

void BufferPoolManager::flush_page(std::size_t page_id) {
    auto it = frames_.find(page_id);
    if (it == frames_.end()) {
        throw std::out_of_range("Page not found in buffer pool");
    }

    if (it->second.page->is_dirty()) {
        page_io_.write_page(page_id, it->second.page->data(), page_size_);
        it->second.page->mark_dirty(false);
    }
}

void BufferPoolManager::flush_all() {
    for (auto& [page_id, frame] : frames_) {
        if (frame.page->is_dirty()) {
            page_io_.write_page(page_id, frame.page->data(), page_size_);
            frame.page->mark_dirty(false);
        }
    }
}

std::size_t BufferPoolManager::cached_pages() const { return frames_.size(); }

void BufferPoolManager::touch(std::size_t page_id) {
    auto it = frames_.find(page_id);
    if (it == frames_.end()) {
        return;
    }

    if (it->second.in_lru) {
        lru_.erase(it->second.lru_it);
        it->second.in_lru = false;
    }
}

void BufferPoolManager::maybe_add_to_lru(std::size_t page_id) {
    auto it = frames_.find(page_id);
    if (it == frames_.end()) {
        return;
    }

    if (it->second.page->pin_count() > 0) {
        return;
    }

    if (it->second.in_lru) {
        lru_.erase(it->second.lru_it);
    }

    lru_.push_front(page_id);
    it->second.lru_it = lru_.begin();
    it->second.in_lru = true;
}

void BufferPoolManager::evict_if_needed() {
    if (frames_.size() < pool_size_) {
        return;
    }

    while (!lru_.empty()) {
        const std::size_t victim_id = lru_.back();
        lru_.pop_back();

        auto it = frames_.find(victim_id);
        if (it == frames_.end()) {
            continue;
        }

        if (it->second.page->pin_count() > 0) {
            continue;
        }

        if (it->second.page->is_dirty()) {
            page_io_.write_page(victim_id, it->second.page->data(), page_size_);
            it->second.page->mark_dirty(false);
        }

        frames_.erase(it);
        return;
    }

    throw std::runtime_error("Buffer pool full and no unpinned page available for eviction");
}

}  // namespace novadb::buffer
