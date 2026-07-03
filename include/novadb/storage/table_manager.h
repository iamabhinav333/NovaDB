#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "novadb/storage/catalog.h"

namespace novadb::storage {

class TableManager {
public:
    explicit TableManager(std::string base_dir);

    bool create_table(const std::string& table_name);
    bool delete_table(const std::string& table_name);

    std::optional<TableMeta> open_table(const std::string& table_name) const;
    std::vector<std::string> list_tables() const;

private:
    std::string base_dir_;
    std::unique_ptr<Catalog> catalog_;

    static bool is_valid_table_name(const std::string& table_name);
    std::string table_file_path(const std::string& table_name) const;
};

}  // namespace novadb::storage
