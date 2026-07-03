#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace novadb::optimizer {

enum class AccessPath {
    Index,
    FullScan,
};

struct TableStatistics {
    std::uint64_t row_count = 0;
    std::uint64_t point_lookups = 0;
    std::uint64_t range_queries = 0;
};

struct PlanRequest {
    bool has_index = true;
    bool point_lookup = false;
    bool range_query = false;
    std::uint64_t key = 0;
    std::uint64_t range_end = 0;
};

struct PlanDecision {
    AccessPath path = AccessPath::Index;
    std::string reason;
};

class QueryOptimizer {
public:
    PlanDecision choose_plan(const PlanRequest& request, const TableStatistics& statistics) const;
};

}  // namespace novadb::optimizer
