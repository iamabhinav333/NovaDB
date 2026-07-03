#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace novadb::storage {

struct TableMeta {
    std::string name;
    std::string file_path;
    std::size_t next_page_id = 0;
};

class Catalog {
public:
    explicit Catalog(std::string catalog_file);

    bool create_table(const std::string& table_name, const std::string& table_file_path);
    bool delete_table(const std::string& table_name);

    std::optional<TableMeta> open_table(const std::string& table_name) const;
    std::vector<TableMeta> list_tables() const;

    bool update_table(const TableMeta& meta);

private:
    std::string catalog_file_;

    std::vector<TableMeta> read_all() const;
    void write_all(const std::vector<TableMeta>& metas) const;
};

}  // namespace novadb::storage
