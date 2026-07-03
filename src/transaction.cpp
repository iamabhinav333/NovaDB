#include "novadb/transaction/transaction.h"

namespace novadb::transaction {

TransactionManager::TransactionManager()
    : state_(TransactionState::Idle), next_transaction_id_(1), current_transaction_id_(0) {}

bool TransactionManager::begin() {
    if (state_ == TransactionState::Active) {
        return false;
    }
    current_transaction_id_ = next_transaction_id_++;
    state_ = TransactionState::Active;
    return true;
}

bool TransactionManager::commit() {
    if (state_ != TransactionState::Active) {
        return false;
    }
    state_ = TransactionState::Committed;
    current_transaction_id_ = 0;
    return true;
}

bool TransactionManager::rollback() {
    if (state_ != TransactionState::Active) {
        return false;
    }
    state_ = TransactionState::RolledBack;
    current_transaction_id_ = 0;
    return true;
}

TransactionState TransactionManager::state() const { return state_; }

std::uint64_t TransactionManager::current_transaction_id() const { return current_transaction_id_; }

bool TransactionManager::active() const { return state_ == TransactionState::Active; }

void TransactionManager::reset() {
    state_ = TransactionState::Idle;
    current_transaction_id_ = 0;
}

}  // namespace novadb::transaction
