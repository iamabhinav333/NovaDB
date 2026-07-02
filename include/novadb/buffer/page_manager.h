#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "novadb/buffer/page.h"
#include "novadb/storage/disk_manager.h"

namespace novadb::buffer {

class IPageIO {
public:
    virtual ~IPageIO() = default;
    virtual std::vector<std::uint8_t> read_page(std::size_t page_id, std::size_t page_size) = 0;
    virtual void write_page(std::size_t page_id, const std::vector<std::uint8_t>& page_data, std::size_t page_size) = 0;
};

class DiskPageIO final : public IPageIO {
public:
    explicit DiskPageIO(novadb::DiskManager& disk_manager);
    DiskPageIO(novadb::DiskManager& disk_manager, std::size_t default_page_size);

    std::vector<std::uint8_t> read_page(std::size_t page_id, std::size_t page_size) override;
    void write_page(std::size_t page_id, const std::vector<std::uint8_t>& page_data, std::size_t page_size) override;

private:
    novadb::DiskManager& disk_manager_;
    std::size_t default_page_size_;
};

class PageManager {
public:
    PageManager(IPageIO& page_io, std::size_t page_size);

    std::shared_ptr<Page> create_page(std::size_t page_id);
    std::shared_ptr<Page> fetch_page(std::size_t page_id);
    void flush_page(std::size_t page_id);
    void flush_all();

private:
    IPageIO& page_io_;
    std::size_t page_size_;
    std::unordered_map<std::size_t, std::shared_ptr<Page>> pages_;
};

}  // namespace novadb::buffer
