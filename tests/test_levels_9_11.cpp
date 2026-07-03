#include <cassert>
#include <filesystem>
#include <string>

#include "novadb/concurrency/lock_manager.h"
#include "novadb/optimizer/query_optimizer.h"
#include "novadb/query/query_engine.h"
#include "novadb/storage/table_manager.h"
#include "novadb/wal/log_record.h"
#include "novadb/wal/write_ahead_log.h"

namespace {

void test_wal_replay_and_recovery() {
    const std::string wal_file = "novadb_test.wal";
    std::filesystem::remove(wal_file);

    novadb::wal::WriteAheadLog wal(wal_file);
    wal.append({1, novadb::wal::LogType::Begin, "users", 0, ""});
    wal.append({1, novadb::wal::LogType::Insert, "users", 1, "Alice"});
    wal.append({1, novadb::wal::LogType::Update, "users", 1, "Bob"});

    std::size_t replayed = 0;
    bool saw_insert = false;
    bool saw_update = false;
    wal.replay([&](const novadb::wal::LogRecord& record) {
        ++replayed;
        if (record.type == novadb::wal::LogType::Insert) {
            saw_insert = true;
        }
        if (record.type == novadb::wal::LogType::Update) {
            saw_update = true;
        }
    });

    assert(replayed == 3);
    assert(saw_insert);
    assert(saw_update);

    wal.clear();
    assert(wal.read_all().empty());

    std::filesystem::remove(wal_file);
}

void test_lock_manager() {
    novadb::concurrency::LockManager locks(std::chrono::milliseconds{100});

    assert(locks.lock_shared("users"));
    assert(locks.lock_shared("users"));
    locks.unlock_shared("users");
    locks.unlock_shared("users");

    assert(locks.lock_exclusive("orders"));
    locks.unlock_exclusive("orders");
}

void test_optimizer() {
    novadb::optimizer::QueryOptimizer optimizer;
    novadb::optimizer::TableStatistics stats{};
    stats.row_count = 1000;

    novadb::optimizer::PlanRequest point{};
    point.has_index = true;
    point.point_lookup = true;
    auto point_plan = optimizer.choose_plan(point, stats);
    assert(point_plan.path == novadb::optimizer::AccessPath::Index);

    novadb::optimizer::PlanRequest range{};
    range.has_index = true;
    range.range_query = true;
    range.key = 1;
    range.range_end = 900;
    auto range_plan = optimizer.choose_plan(range, stats);
    assert(range_plan.path == novadb::optimizer::AccessPath::FullScan ||
           range_plan.path == novadb::optimizer::AccessPath::Index);

    novadb::optimizer::PlanRequest no_index{};
    no_index.has_index = false;
    auto no_index_plan = optimizer.choose_plan(no_index, stats);
    assert(no_index_plan.path == novadb::optimizer::AccessPath::FullScan);
}

void test_query_recovery_path() {
    const std::string base_dir = "novadb_level_9_11_data";
    std::filesystem::remove_all(base_dir);

    novadb::storage::TableManager tables(base_dir);
    assert(tables.create_table("users"));

    novadb::query::QueryEngine engine(tables);
    assert(engine.execute("INSERT users 1 Alice").success);
    assert(engine.execute("BEGIN").success);
    assert(engine.execute("UPDATE users 1 Bob").success);
    assert(engine.execute("ROLLBACK").success);

    auto recovered = engine.execute("FIND users 1");
    assert(recovered.success);
    assert(recovered.value.has_value());
    assert(*recovered.value == "Alice");

    std::filesystem::remove_all(base_dir);
}

}  // namespace

int main() {
    test_wal_replay_and_recovery();
    test_lock_manager();
    test_optimizer();
    test_query_recovery_path();
    return 0;
}
