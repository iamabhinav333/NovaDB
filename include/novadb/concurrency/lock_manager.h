#pragma once

#include <condition_variable>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace novadb::concurrency {

enum class LockMode {
    Shared,
    Exclusive,
};

class LockManager {
public:
    explicit LockManager(std::chrono::milliseconds default_timeout = std::chrono::milliseconds{2000});

    bool lock_shared(const std::string& resource, std::chrono::milliseconds timeout = std::chrono::milliseconds{-1});
    bool lock_exclusive(const std::string& resource, std::chrono::milliseconds timeout = std::chrono::milliseconds{-1});
    void unlock_shared(const std::string& resource);
    void unlock_exclusive(const std::string& resource);

private:
    struct LockState {
        std::size_t shared_holders = 0;
        bool exclusive = false;
    };

    std::chrono::milliseconds default_timeout_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::unordered_map<std::string, LockState> locks_;

    bool wait_for_lock(const std::string& resource, LockMode mode, std::chrono::milliseconds timeout);
};

}  // namespace novadb::concurrency
