#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace novadb {

class DiskManager {
public:
    explicit DiskManager(std::string db_file);
    ~DiskManager();

    DiskManager(const DiskManager&) = delete;
    DiskManager& operator=(const DiskManager&) = delete;

    std::vector<std::uint8_t> read(std::size_t offset, std::size_t size);
    void write(std::size_t offset, const std::vector<std::uint8_t>& data);

    std::vector<std::uint8_t> read_page(std::size_t page_id, std::size_t page_size);
    void write_page(std::size_t page_id, const std::vector<std::uint8_t>& page_data, std::size_t page_size);

    const std::string& file_path() const;

private:
    void ensure_file_exists();
    std::size_t file_size();

    std::string db_file_;
    std::fstream file_;
    std::mutex mutex_;
};

}  // namespace novadb
