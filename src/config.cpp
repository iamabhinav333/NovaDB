#include "novadb/common/config.h"

#include <cstdlib>
#include <limits>

namespace novadb {

namespace {

std::size_t parse_size_or_default(const char* value, std::size_t default_value) {
    if (value == nullptr) {
        return default_value;
    }

    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(value, &end, 10);
    if (end == value || *end != '\0') {
        return default_value;
    }
    if (parsed > std::numeric_limits<std::size_t>::max()) {
        return default_value;
    }

    return static_cast<std::size_t>(parsed);
}

}  // namespace

bool Config::is_valid() const {
    return page_size > 0 && db_file.size() > 0 && cache_pages > 0;
}

Config load_config_from_env() {
    Config cfg{};

    if (const char* db_file = std::getenv("NOVADB_DB_FILE"); db_file != nullptr) {
        cfg.db_file = db_file;
    }

    cfg.page_size = parse_size_or_default(std::getenv("NOVADB_PAGE_SIZE"), cfg.page_size);
    cfg.cache_pages = parse_size_or_default(std::getenv("NOVADB_CACHE_PAGES"), cfg.cache_pages);

    return cfg;
}

}  // namespace novadb
