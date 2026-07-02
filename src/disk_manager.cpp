#include "novadb/storage/disk_manager.h"

#include <algorithm>
#include <stdexcept>

namespace novadb {

DiskManager::DiskManager(std::string db_file) : db_file_(std::move(db_file)) {
    ensure_file_exists();
    file_.open(db_file_, std::ios::binary | std::ios::in | std::ios::out);
    if (!file_.is_open()) {
        throw std::runtime_error("Failed to open database file: " + db_file_);
    }
}

DiskManager::~DiskManager() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

std::vector<std::uint8_t> DiskManager::read(std::size_t offset, std::size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::uint8_t> out(size, 0);
    const std::size_t current_size = file_size();
    if (offset >= current_size || size == 0) {
        return out;
    }

    const std::size_t readable = std::min(size, current_size - offset);
    file_.clear();
    file_.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    file_.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(readable));

    return out;
}

void DiskManager::write(std::size_t offset, const std::vector<std::uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (data.empty()) {
        return;
    }

    file_.clear();
    file_.seekp(static_cast<std::streamoff>(offset), std::ios::beg);
    file_.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    file_.flush();

    if (!file_) {
        throw std::runtime_error("Disk write failed for file: " + db_file_);
    }
}

std::vector<std::uint8_t> DiskManager::read_page(std::size_t page_id, std::size_t page_size) {
    return read(page_id * page_size, page_size);
}

void DiskManager::write_page(std::size_t page_id, const std::vector<std::uint8_t>& page_data, std::size_t page_size) {
    if (page_data.size() != page_size) {
        throw std::invalid_argument("write_page expects fixed-size page data");
    }
    write(page_id * page_size, page_data);
}

const std::string& DiskManager::file_path() const { return db_file_; }

void DiskManager::ensure_file_exists() {
    std::ifstream in(db_file_, std::ios::binary);
    if (in.good()) {
        return;
    }

    std::ofstream out(db_file_, std::ios::binary);
    if (!out.good()) {
        throw std::runtime_error("Failed to create database file: " + db_file_);
    }
}

std::size_t DiskManager::file_size() {
    file_.clear();
    const auto current_pos = file_.tellg();
    file_.seekg(0, std::ios::end);
    const auto size = file_.tellg();
    file_.seekg(current_pos, std::ios::beg);

    if (size < 0) {
        return 0;
    }
    return static_cast<std::size_t>(size);
}

}  // namespace novadb
