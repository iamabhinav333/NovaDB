#include "novadb/storage/catalog.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace novadb::storage {

Catalog::Catalog(std::string catalog_file) : catalog_file_(std::move(catalog_file)) {
    std::ifstream in(catalog_file_);
    if (!in.good()) {
        std::ofstream out(catalog_file_);
        if (!out.good()) {
            throw std::runtime_error("Failed to create catalog file: " + catalog_file_);
        }
    }
}

bool Catalog::create_table(const std::string& table_name, const std::string& table_file_path) {
    auto metas = read_all();
    const auto exists = std::any_of(metas.begin(), metas.end(), [&](const TableMeta& m) { return m.name == table_name; });
    if (exists) {
        return false;
    }

    metas.push_back(TableMeta{table_name, table_file_path, 0});
    write_all(metas);
    return true;
}

bool Catalog::delete_table(const std::string& table_name) {
    auto metas = read_all();
    const auto old_size = metas.size();

    metas.erase(std::remove_if(metas.begin(), metas.end(), [&](const TableMeta& m) { return m.name == table_name; }),
               metas.end());

    if (metas.size() == old_size) {
        return false;
    }

    write_all(metas);
    return true;
}

std::optional<TableMeta> Catalog::open_table(const std::string& table_name) const {
    const auto metas = read_all();
    const auto it = std::find_if(metas.begin(), metas.end(), [&](const TableMeta& m) { return m.name == table_name; });
    if (it == metas.end()) {
        return std::nullopt;
    }
    return *it;
}

std::vector<TableMeta> Catalog::list_tables() const { return read_all(); }

bool Catalog::update_table(const TableMeta& meta) {
    auto metas = read_all();
    const auto it = std::find_if(metas.begin(), metas.end(), [&](const TableMeta& m) { return m.name == meta.name; });
    if (it == metas.end()) {
        return false;
    }

    *it = meta;
    write_all(metas);
    return true;
}

std::vector<TableMeta> Catalog::read_all() const {
    std::ifstream in(catalog_file_);
    std::vector<TableMeta> metas;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream ss(line);
        std::string name;
        std::string file_path;
        std::string next_page;

        if (!std::getline(ss, name, '|')) {
            continue;
        }
        if (!std::getline(ss, file_path, '|')) {
            continue;
        }
        if (!std::getline(ss, next_page, '|')) {
            continue;
        }

        TableMeta meta{};
        meta.name = name;
        meta.file_path = file_path;
        meta.next_page_id = static_cast<std::size_t>(std::stoull(next_page));
        metas.push_back(std::move(meta));
    }

    return metas;
}

void Catalog::write_all(const std::vector<TableMeta>& metas) const {
    std::ofstream out(catalog_file_, std::ios::trunc);
    if (!out.good()) {
        throw std::runtime_error("Failed to write catalog file: " + catalog_file_);
    }

    for (const auto& meta : metas) {
        out << meta.name << '|' << meta.file_path << '|' << meta.next_page_id << '\n';
    }
}

}  // namespace novadb::storage
