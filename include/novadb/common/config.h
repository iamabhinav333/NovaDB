#pragma once

#include <cstddef>
#include <string>

namespace novadb {

struct Config {
    std::size_t page_size = 4096;
    std::string db_file = "database.db";
    std::size_t cache_pages = 128;

    bool is_valid() const;
};

Config load_config_from_env();

}  // namespace novadb
