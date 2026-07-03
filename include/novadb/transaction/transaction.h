#pragma once

#include <cstdint>

namespace novadb::transaction {

enum class TransactionState {
    Idle,
    Active,
    Committed,
    RolledBack,
};

class TransactionManager {
public:
    TransactionManager();

    bool begin();
    bool commit();
    bool rollback();

    TransactionState state() const;
    std::uint64_t current_transaction_id() const;
    bool active() const;

    void reset();

private:
    TransactionState state_;
    std::uint64_t next_transaction_id_;
    std::uint64_t current_transaction_id_;
};

}  // namespace novadb::transaction
