#include <cassert>
#include <cstdio>
#include <vector>

#include "novadb/buffer/page_manager.h"
#include "novadb/storage/disk_manager.h"

int main() {
    const char* test_db = "novadb_test.db";
    {
        novadb::DiskManager disk(test_db);
        const std::vector<std::uint8_t> bytes{1, 2, 3, 4};
        disk.write(0, bytes);

        const auto read_back = disk.read(0, bytes.size());
        assert(read_back[0] == 1);
        assert(read_back[3] == 4);

        novadb::buffer::DiskPageIO page_io(disk, 4096);
        novadb::buffer::PageManager pm(page_io, 4096);

        auto page = pm.create_page(5);
        page->mutable_data()[0] = 42;
        pm.flush_page(5);

        auto fetched = pm.fetch_page(5);
        assert(fetched->data()[0] == 42);
    }

    std::remove(test_db);
    return 0;
}
