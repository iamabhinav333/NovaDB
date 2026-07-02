#include <iostream>

#include "novadb/buffer/page_manager.h"
#include "novadb/common/config.h"
#include "novadb/common/logger.h"
#include "novadb/storage/disk_manager.h"

int main() {
    using namespace novadb;

    Logger::set_level(LogLevel::Info);

    Config cfg{};
    cfg.db_file = "database.db";
    cfg.page_size = 4096;
    cfg.cache_pages = 128;

    DiskManager disk(cfg.db_file);
    buffer::DiskPageIO page_io(disk, cfg.page_size);
    buffer::PageManager page_manager(page_io, cfg.page_size);

    auto page = page_manager.create_page(0);
    page->mutable_data()[0] = 'N';
    page->mutable_data()[1] = 'D';
    page->mutable_data()[2] = 'B';
    page_manager.flush_page(0);

    auto fetched = page_manager.fetch_page(0);
    std::cout << "NovaDB page[0..2]: " << fetched->data()[0] << fetched->data()[1] << fetched->data()[2] << '\n';

    Logger::info("NovaDB foundation initialized successfully.");
    return 0;
}
