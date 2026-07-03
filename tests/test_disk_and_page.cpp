#include <cassert>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "novadb/buffer/buffer_pool.h"
#include "novadb/buffer/page_manager.h"
#include "novadb/storage/record_page.h"
#include "novadb/storage/disk_manager.h"
#include "novadb/storage/table_manager.h"

namespace {

void test_disk_and_page_basics() {
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
}

void test_record_page_operations() {
    const char* test_db = "novadb_record_test.db";
    {
        novadb::DiskManager disk(test_db);
        novadb::buffer::DiskPageIO page_io(disk, 4096);
        novadb::buffer::PageManager pm(page_io, 4096);

        auto page = pm.create_page(1);
        novadb::storage::RecordPage rp(*page);

        const auto slot = rp.insert(novadb::storage::Record{10, "first"});
        assert(slot.has_value());

        auto found_slot = rp.search_by_key(10);
        assert(found_slot.has_value());
        assert(*found_slot == *slot);

        bool updated = rp.update(*slot, novadb::storage::Record{10, "first-updated"});
        assert(updated);

        auto rec = rp.read(*slot);
        assert(rec.has_value());
        assert(rec->value == "first-updated");

        bool deleted = rp.erase(*slot);
        assert(deleted);
        auto missing = rp.read(*slot);
        assert(!missing.has_value());

        assert(rp.free_space() > 0);
    }
    std::remove(test_db);
}

void test_table_manager_and_catalog() {
    const std::string base_dir = "novadb_data_test";
    std::filesystem::remove_all(base_dir);

    novadb::storage::TableManager tm(base_dir);
    bool created_users = tm.create_table("users");
    bool created_orders = tm.create_table("orders");
    assert(created_users);
    assert(created_orders);

    auto list = tm.list_tables();
    assert(list.size() == 2);

    auto users = tm.open_table("users");
    assert(users.has_value());

    bool deleted = tm.delete_table("orders");
    assert(deleted);
    auto orders = tm.open_table("orders");
    assert(!orders.has_value());

    std::filesystem::remove_all(base_dir);
}

void test_buffer_pool_lru_and_flush() {
    const char* test_db = "novadb_buffer_pool_test.db";
    {
        novadb::DiskManager disk(test_db);
        novadb::buffer::DiskPageIO page_io(disk, 4096);
        novadb::buffer::BufferPoolManager bpm(page_io, 4096, 2);

        auto p0 = bpm.create_page(0);
        p0->mutable_data()[0] = 11;
        bpm.unpin_page(0, true);

        auto p1 = bpm.create_page(1);
        p1->mutable_data()[0] = 22;
        bpm.unpin_page(1, true);

        auto p2 = bpm.create_page(2);
        p2->mutable_data()[0] = 33;
        bpm.unpin_page(2, true);

        assert(bpm.cached_pages() == 2);
        bpm.flush_all();

        const auto page0 = disk.read_page(0, 4096);
        const auto page1 = disk.read_page(1, 4096);
        const auto page2 = disk.read_page(2, 4096);
        assert(page0[0] == 11);
        assert(page1[0] == 22);
        assert(page2[0] == 33);
    }
    std::remove(test_db);
}

}  // namespace

int main() {
    test_disk_and_page_basics();
    test_record_page_operations();
    test_table_manager_and_catalog();
    test_buffer_pool_lru_and_flush();
    return 0;
}
