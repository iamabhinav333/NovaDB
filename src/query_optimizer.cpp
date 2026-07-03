#include "novadb/optimizer/query_optimizer.h"

namespace novadb::optimizer {

PlanDecision QueryOptimizer::choose_plan(const PlanRequest& request, const TableStatistics& statistics) const {
    PlanDecision decision{};

    if (!request.has_index) {
        decision.path = AccessPath::FullScan;
        decision.reason = "No index available";
        return decision;
    }

    if (request.point_lookup) {
        decision.path = AccessPath::Index;
        decision.reason = "Point lookup or key mutation uses index";
        return decision;
    }

    if (request.range_query) {
        if (statistics.row_count > 0 && request.range_end > request.key &&
            (request.range_end - request.key) > (statistics.row_count / 2)) {
            decision.path = AccessPath::FullScan;
            decision.reason = "Wide range prefers scan for small tables";
        } else {
            decision.path = AccessPath::Index;
            decision.reason = "Narrow range uses index";
        }
        return decision;
    }

    decision.path = AccessPath::Index;
    decision.reason = "Default to index";
    return decision;
}

}  // namespace novadb::optimizer
