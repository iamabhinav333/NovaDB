#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>

#include "novadb/storage/disk_manager.h"

int main() {
    novadb::DiskManager disk("bench.db");
    std::vector<std::uint8_t> page(4096, 7);

    constexpr std::size_t iterations = 1000;
    const auto start = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < iterations; ++i) {
        disk.write_page(i, page, 4096);
    }

    const auto end = std::chrono::steady_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Wrote " << iterations << " pages in " << ms << " ms\n";
    return 0;
}
