#pragma once

#include <cstddef>
#include <list>
#include <memory>
#include <unordered_map>

#include "novadb/buffer/page.h"
#include "novadb/buffer/page_manager.h"

namespace novadb::buffer {

class BufferPoolManager {
public:
    BufferPoolManager(IPageIO& page_io, std::size_t page_size, std::size_t pool_size);

    std::shared_ptr<Page> fetch_page(std::size_t page_id);
    std::shared_ptr<Page> create_page(std::size_t page_id);

    bool unpin_page(std::size_t page_id, bool mark_dirty);
    void flush_page(std::size_t page_id);
    void flush_all();

    std::size_t cached_pages() const;

private:
    struct Frame {
        std::shared_ptr<Page> page;
        std::list<std::size_t>::iterator lru_it;
        bool in_lru = false;
    };

    IPageIO& page_io_;
    std::size_t page_size_;
    std::size_t pool_size_;

    std::unordered_map<std::size_t, Frame> frames_;
    std::list<std::size_t> lru_;

    void touch(std::size_t page_id);
    void maybe_add_to_lru(std::size_t page_id);
    void evict_if_needed();
};

}  // namespace novadb::buffer
