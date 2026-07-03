#include "novadb/concurrency/lock_manager.h"

#include <algorithm>

namespace novadb::concurrency {

LockManager::LockManager(std::chrono::milliseconds default_timeout) : default_timeout_(default_timeout) {}

bool LockManager::lock_shared(const std::string& resource, std::chrono::milliseconds timeout) {
    return wait_for_lock(resource, LockMode::Shared, timeout);
}

bool LockManager::lock_exclusive(const std::string& resource, std::chrono::milliseconds timeout) {
    return wait_for_lock(resource, LockMode::Exclusive, timeout);
}

void LockManager::unlock_shared(const std::string& resource) {
    std::lock_guard<std::mutex> guard(mutex_);
    auto it = locks_.find(resource);
    if (it == locks_.end() || it->second.shared_holders == 0) {
        return;
    }

    --it->second.shared_holders;
    if (it->second.shared_holders == 0 && !it->second.exclusive) {
        locks_.erase(it);
    }
    cv_.notify_all();
}

void LockManager::unlock_exclusive(const std::string& resource) {
    std::lock_guard<std::mutex> guard(mutex_);
    auto it = locks_.find(resource);
    if (it == locks_.end() || !it->second.exclusive) {
        return;
    }

    it->second.exclusive = false;
    if (it->second.shared_holders == 0) {
        locks_.erase(it);
    }
    cv_.notify_all();
}

bool LockManager::wait_for_lock(const std::string& resource, LockMode mode, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    const auto effective_timeout = timeout.count() < 0 ? default_timeout_ : timeout;

    const auto predicate = [&]() {
        auto& state = locks_[resource];
        if (mode == LockMode::Shared) {
            return !state.exclusive;
        }
        return !state.exclusive && state.shared_holders == 0;
    };

    if (!cv_.wait_for(lock, effective_timeout, predicate)) {
        return false;
    }

    auto& state = locks_[resource];
    if (mode == LockMode::Shared) {
        ++state.shared_holders;
    } else {
        state.exclusive = true;
    }
    return true;
}

}  // namespace novadb::concurrency
