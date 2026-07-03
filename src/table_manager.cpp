#include "novadb/storage/table_manager.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <utility>

namespace novadb::storage {

TableManager::TableManager(std::string base_dir) : base_dir_(std::move(base_dir)) {
    std::filesystem::create_directories(base_dir_);
    std::filesystem::create_directories(base_dir_ + "/tables");
    catalog_ = std::make_unique<Catalog>(base_dir_ + "/catalog.meta");
}

bool TableManager::create_table(const std::string& table_name) {
    if (!is_valid_table_name(table_name)) {
        return false;
    }

    const std::string path = table_file_path(table_name);
    if (!catalog_->create_table(table_name, path)) {
        return false;
    }

    std::ofstream out(path, std::ios::binary);
    return out.good();
}

bool TableManager::delete_table(const std::string& table_name) {
    const auto meta = catalog_->open_table(table_name);
    if (!meta.has_value()) {
        return false;
    }

    const bool deleted_from_catalog = catalog_->delete_table(table_name);
    std::error_code ec;
    std::filesystem::remove(meta->file_path, ec);
    return deleted_from_catalog;
}

std::optional<TableMeta> TableManager::open_table(const std::string& table_name) const {
    return catalog_->open_table(table_name);
}

std::vector<std::string> TableManager::list_tables() const {
    const auto metas = catalog_->list_tables();
    std::vector<std::string> names;
    names.reserve(metas.size());

    for (const auto& m : metas) {
        names.push_back(m.name);
    }

    return names;
}

bool TableManager::is_valid_table_name(const std::string& table_name) {
    if (table_name.empty()) {
        return false;
    }

    for (char c : table_name) {
        const bool valid = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
        if (!valid) {
            return false;
        }
    }
    return true;
}

std::string TableManager::table_file_path(const std::string& table_name) const {
    return base_dir_ + "/tables/" + table_name + ".tbl";
}

}  // namespace novadb::storage
