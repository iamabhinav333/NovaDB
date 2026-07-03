#include <cassert>
#include <filesystem>
#include <string>
#include <vector>

#include "novadb/index/bplustree.h"
#include "novadb/query/query_engine.h"
#include "novadb/storage/table_manager.h"

namespace {

void test_bplustree_operations() {
    novadb::index::BPlusTree tree(3);

    assert(tree.insert(1, "one"));
    assert(tree.insert(2, "two"));
    assert(tree.insert(3, "three"));
    assert(tree.insert(4, "four"));
    assert(tree.insert(5, "five"));
    assert(tree.insert(6, "six"));

    auto found = tree.search(4);
    assert(found.has_value());
    assert(*found == "four");

    auto range = tree.range_query(2, 5);
    assert(range.size() == 4);
    assert(range.front().first == 2);
    assert(range.back().first == 5);

    assert(tree.remove(4));
    assert(!tree.search(4).has_value());

    auto copy = tree;
    assert(copy.search(5).has_value());
    assert(copy.size() == tree.size());
}

void test_query_engine_and_transactions() {
    const std::string base_dir = "novadb_query_test_data";
    std::filesystem::remove_all(base_dir);

    novadb::storage::TableManager tables(base_dir);
    assert(tables.create_table("users"));

    novadb::query::QueryEngine engine(tables);

    auto insert_result = engine.execute("INSERT users 1 Alice");
    assert(insert_result.success);

    auto find_result = engine.execute("FIND users 1");
    assert(find_result.success);
    assert(find_result.value.has_value());
    assert(*find_result.value == "Alice");

    auto begin_result = engine.execute("BEGIN");
    assert(begin_result.success);
    assert(engine.transaction_state() == novadb::transaction::TransactionState::Active);

    auto update_result = engine.execute("UPDATE users 1 Bob");
    assert(update_result.success);

    auto rollback_result = engine.execute("ROLLBACK");
    assert(rollback_result.success);
    assert(engine.transaction_state() == novadb::transaction::TransactionState::RolledBack);

    auto after_rollback = engine.execute("FIND users 1");
    assert(after_rollback.success);
    assert(after_rollback.value.has_value());
    assert(*after_rollback.value == "Alice");

    auto begin_again = engine.execute("BEGIN");
    assert(begin_again.success);

    auto update_again = engine.execute("UPDATE users 1 Carol");
    assert(update_again.success);

    auto commit_result = engine.execute("COMMIT");
    assert(commit_result.success);
    assert(engine.transaction_state() == novadb::transaction::TransactionState::Committed);

    auto after_commit = engine.execute("FIND users 1");
    assert(after_commit.success);
    assert(after_commit.value.has_value());
    assert(*after_commit.value == "Carol");

    auto delete_result = engine.execute("DELETE users 1");
    assert(delete_result.success);
    auto missing = engine.execute("FIND users 1");
    assert(!missing.success);

    std::filesystem::remove_all(base_dir);
}

}  // namespace

int main() {
    test_bplustree_operations();
    test_query_engine_and_transactions();
    return 0;
}
