#include "novadb/query/query_engine.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <utility>

namespace novadb::query {

namespace {

std::string to_upper(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return text;
}

std::string trim_left(std::string text) {
    const auto it = std::find_if_not(text.begin(), text.end(), [](unsigned char c) { return std::isspace(c) != 0; });
    text.erase(text.begin(), it);
    return text;
}

std::vector<std::string> split_words(std::string_view text) {
    std::istringstream stream{std::string(text)};
    std::vector<std::string> words;
    std::string word;
    while (stream >> word) {
        words.push_back(word);
    }
    return words;
}

}  // namespace

std::optional<QueryRequest> QueryParser::parse(std::string_view text) {
    const auto words = split_words(text);
    if (words.empty()) {
        return std::nullopt;
    }

    QueryRequest request{};
    const std::string op = to_upper(words[0]);

    if (op == "BEGIN") {
        request.type = QueryType::Begin;
        return request;
    }
    if (op == "COMMIT") {
        request.type = QueryType::Commit;
        return request;
    }
    if (op == "ROLLBACK") {
        request.type = QueryType::Rollback;
        return request;
    }

    if (words.size() < 3) {
        return std::nullopt;
    }

    request.table = words[1];
    request.key = std::stoull(words[2]);

    if (op == "INSERT" || op == "UPDATE") {
        request.type = op == "INSERT" ? QueryType::Insert : QueryType::Update;
        const auto first_key_pos = text.find(words[2]);
        if (first_key_pos != std::string_view::npos) {
            const auto value_pos = text.find_first_not_of(' ', first_key_pos + words[2].size());
            if (value_pos != std::string_view::npos) {
                request.value = trim_left(std::string(text.substr(value_pos)));
            }
        }
        return request;
    }

    if (op == "FIND") {
        request.type = QueryType::Find;
        return request;
    }

    if (op == "DELETE") {
        request.type = QueryType::Delete;
        return request;
    }

    if (op == "RANGE") {
        if (words.size() < 4) {
            return std::nullopt;
        }
        request.type = QueryType::Range;
        request.range_end = std::stoull(words[3]);
        return request;
    }

    return std::nullopt;
}

QueryEngine::QueryEngine(storage::TableManager& table_manager) : table_manager_(table_manager) {}

QueryResult QueryEngine::execute(std::string_view text) {
    const auto parsed = QueryParser::parse(text);
    if (!parsed.has_value()) {
        return {false, "Invalid command", std::nullopt, {}};
    }
    return execute_request(*parsed);
}

transaction::TransactionState QueryEngine::transaction_state() const { return transactions_.state(); }

std::uint64_t QueryEngine::current_transaction_id() const { return transactions_.current_transaction_id(); }

QueryResult QueryEngine::execute_request(const QueryRequest& request) {
    switch (request.type) {
        case QueryType::Begin: {
            if (!transactions_.begin()) {
                return {false, "Transaction already active", std::nullopt, {}};
            }
            transaction_snapshot_ = tables_;
            return {true, "Transaction started", std::nullopt, {}};
        }
        case QueryType::Commit: {
            if (!transactions_.commit()) {
                return {false, "No active transaction to commit", std::nullopt, {}};
            }
            transaction_snapshot_.clear();
            return {true, "Transaction committed", std::nullopt, {}};
        }
        case QueryType::Rollback: {
            if (!transactions_.rollback()) {
                return {false, "No active transaction to roll back", std::nullopt, {}};
            }
            tables_ = transaction_snapshot_;
            transaction_snapshot_.clear();
            return {true, "Transaction rolled back", std::nullopt, {}};
        }
        case QueryType::Insert:
        case QueryType::Find:
        case QueryType::Update:
        case QueryType::Delete:
        case QueryType::Range:
            break;
        default:
            return {false, "Unsupported command", std::nullopt, {}};
    }

    if (!table_exists(request.table)) {
        return {false, "Unknown table: " + request.table, std::nullopt, {}};
    }

    auto& index = tables_[request.table];

    switch (request.type) {
        case QueryType::Insert: {
            if (!index.insert(request.key, request.value)) {
                return {false, "Key already exists", std::nullopt, {}};
            }
            return {true, "Inserted", std::nullopt, {}};
        }
        case QueryType::Find: {
            const auto value = index.search(request.key);
            if (!value.has_value()) {
                return {false, "Not found", std::nullopt, {}};
            }
            return {true, "Found", value, {}};
        }
        case QueryType::Update: {
            if (!index.update(request.key, request.value)) {
                return {false, "Key not found", std::nullopt, {}};
            }
            return {true, "Updated", std::nullopt, {}};
        }
        case QueryType::Delete: {
            if (!index.remove(request.key)) {
                return {false, "Key not found", std::nullopt, {}};
            }
            return {true, "Deleted", std::nullopt, {}};
        }
        case QueryType::Range: {
            return {true, "Range query complete", std::nullopt, index.range_query(request.key, request.range_end)};
        }
        default:
            return {false, "Unsupported command", std::nullopt, {}};
    }
}

bool QueryEngine::table_exists(const std::string& table_name) const {
    return table_manager_.open_table(table_name).has_value();
}

}  // namespace novadb::query
