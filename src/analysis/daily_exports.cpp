//
// Created by rain on 07/08/26.
//
#include "src/analysis/daily_exports.hpp"

#include <algorithm>
#include <utility>

namespace sift {
DailyExportCounts count_daily_exports(std::span<const Event> events) {
    DailyExportCounts counts;
    std::map<std::string,
             std::pair<std::chrono::sys_days, std::chrono::sys_days>> spans;

    for (const auto& event : events) {
        const auto day = std::chrono::floor<std::chrono::days>(event.timestamp);
        auto [span, inserted] = spans.try_emplace(
            event.primary_entity_id, day, day);
        if (!inserted) {
            span->second.first = std::min(span->second.first, day);
            span->second.second = std::max(span->second.second, day);
        }
        auto& count = counts[event.primary_entity_id][day];
        if (event.event_type == "export") {
            ++count;
        }
    }

    for (const auto& [entity_id, span] : spans) {
        for (auto day = span.first; day <= span.second;
             day += std::chrono::days{1}) {
            counts[entity_id].try_emplace(day, 0);
        }
    }
    return counts;
}
}
