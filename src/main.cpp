#include <iostream>
#include <optional>

#include "novadb/concurrency/lock_manager.h"
#include "novadb/buffer/buffer_pool.h"
#include "novadb/buffer/page_manager.h"
#include "novadb/common/config.h"
#include "novadb/common/logger.h"
#include "novadb/query/query_engine.h"
#include "novadb/optimizer/query_optimizer.h"
#include "novadb/storage/record_page.h"
#include "novadb/storage/disk_manager.h"
#include "novadb/storage/table_manager.h"
#include "novadb/wal/write_ahead_log.h"

int main() {
    using namespace novadb;

    Logger::set_level(LogLevel::Info);

    Config cfg{};
    cfg.db_file = "database.db";
    cfg.page_size = 4096;
    cfg.cache_pages = 128;

    DiskManager disk(cfg.db_file);
    buffer::DiskPageIO page_io(disk, cfg.page_size);
    buffer::BufferPoolManager buffer_pool(page_io, cfg.page_size, cfg.cache_pages);

    storage::TableManager tables("data");
    tables.create_table("users");

    query::QueryEngine engine(tables);
    wal::WriteAheadLog wal("novadb.wal");
    wal.append({1, wal::LogType::Begin, "users", 0, ""});
    wal.append({1, wal::LogType::Insert, "users", 1, "alice"});
    engine.execute("INSERT users 1 alice");
    auto fetched_value = engine.execute("FIND users 1");
    if (fetched_value.value.has_value()) {
        std::cout << "Query layer find: " << *fetched_value.value << '\n';
    }
    engine.execute("BEGIN");
    engine.execute("UPDATE users 1 alice_tx");
    engine.execute("ROLLBACK");

    concurrency::LockManager lock_manager;
    lock_manager.lock_shared("users");
    lock_manager.unlock_shared("users");

    optimizer::QueryOptimizer optimizer;
    optimizer::PlanRequest plan_request{};
    plan_request.has_index = true;
    plan_request.point_lookup = true;
    auto plan = optimizer.choose_plan(plan_request, optimizer::TableStatistics{});
    std::cout << "Optimizer chose: " << (plan.path == optimizer::AccessPath::Index ? "Index" : "Scan") << '\n';

    auto page = buffer_pool.create_page(0);
    storage::RecordPage record_page(*page);
    const auto slot = record_page.insert(storage::Record{1, "alice"});
    buffer_pool.unpin_page(0, true);
    buffer_pool.flush_page(0);

    auto fetched = buffer_pool.fetch_page(0);
    storage::RecordPage fetched_record_page(*fetched);
    auto loaded = slot.has_value() ? fetched_record_page.read(*slot) : std::nullopt;

    if (loaded.has_value()) {
        std::cout << "Loaded record key=" << loaded->key << " value=" << loaded->value << '\n';
    }
    buffer_pool.unpin_page(0, false);

    Logger::info("NovaDB levels 0-11 initialized successfully.");
    return 0;
}
