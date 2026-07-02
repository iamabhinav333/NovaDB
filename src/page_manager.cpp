#include "novadb/buffer/page_manager.h"

#include <stdexcept>

namespace novadb::buffer {

DiskPageIO::DiskPageIO(novadb::DiskManager& disk_manager)
    : disk_manager_(disk_manager), default_page_size_(4096) {}

DiskPageIO::DiskPageIO(novadb::DiskManager& disk_manager, std::size_t default_page_size)
    : disk_manager_(disk_manager), default_page_size_(default_page_size) {}

std::vector<std::uint8_t> DiskPageIO::read_page(std::size_t page_id, std::size_t page_size) {
    const std::size_t effective_page_size = page_size == 0 ? default_page_size_ : page_size;
    return disk_manager_.read_page(page_id, effective_page_size);
}

void DiskPageIO::write_page(std::size_t page_id, const std::vector<std::uint8_t>& page_data, std::size_t page_size) {
    const std::size_t effective_page_size = page_size == 0 ? default_page_size_ : page_size;
    disk_manager_.write_page(page_id, page_data, effective_page_size);
}

PageManager::PageManager(IPageIO& page_io, std::size_t page_size) : page_io_(page_io), page_size_(page_size) {}

std::shared_ptr<Page> PageManager::create_page(std::size_t page_id) {
    auto page = std::make_shared<Page>(page_id, page_size_);
    page->pin();
    pages_[page_id] = page;
    return page;
}

std::shared_ptr<Page> PageManager::fetch_page(std::size_t page_id) {
    if (auto it = pages_.find(page_id); it != pages_.end()) {
        it->second->pin();
        return it->second;
    }

    auto page = std::make_shared<Page>(page_id, page_size_);
    page->overwrite_data(page_io_.read_page(page_id, page_size_));
    page->pin();
    pages_[page_id] = page;

    return page;
}

void PageManager::flush_page(std::size_t page_id) {
    auto it = pages_.find(page_id);
    if (it == pages_.end()) {
        throw std::out_of_range("Page not found in buffer");
    }

    auto& page = it->second;
    if (page->is_dirty()) {
        page_io_.write_page(page->page_id(), page->data(), page_size_);
        page->mark_dirty(false);
    }
}

void PageManager::flush_all() {
    for (auto& [page_id, page] : pages_) {
        (void)page_id;
        if (page->is_dirty()) {
            page_io_.write_page(page->page_id(), page->data(), page_size_);
            page->mark_dirty(false);
        }
    }
}

}  // namespace novadb::buffer
