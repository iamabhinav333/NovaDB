#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "novadb/index/bplustree.h"
#include "novadb/storage/table_manager.h"
#include "novadb/transaction/transaction.h"

namespace novadb::query {

enum class QueryType {
    Invalid,
    Insert,
    Find,
    Update,
    Delete,
    Range,
    Begin,
    Commit,
    Rollback,
};

struct QueryRequest {
    QueryType type = QueryType::Invalid;
    std::string table;
    std::uint64_t key = 0;
    std::uint64_t range_end = 0;
    std::string value;
};

struct QueryResult {
    bool success = false;
    std::string message;
    std::optional<std::string> value;
    std::vector<std::pair<std::uint64_t, std::string>> rows;
};

class QueryParser {
public:
    static std::optional<QueryRequest> parse(std::string_view text);
};

class QueryEngine {
public:
    explicit QueryEngine(storage::TableManager& table_manager);

    QueryResult execute(std::string_view text);

    transaction::TransactionState transaction_state() const;
    std::uint64_t current_transaction_id() const;

private:
    storage::TableManager& table_manager_;
    std::unordered_map<std::string, index::BPlusTree> tables_;
    std::unordered_map<std::string, index::BPlusTree> transaction_snapshot_;
    transaction::TransactionManager transactions_;

    QueryResult execute_request(const QueryRequest& request);
    bool table_exists(const std::string& table_name) const;
};

}  // namespace novadb::query
