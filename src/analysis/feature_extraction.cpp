//
// Created by rain on 17/09/26.
//
#include "src/analysis/feature_extraction.hpp"

#include <algorithm>
#include <numeric>

#include "src/analysis/daily_exports.hpp"

namespace sift {

ExportHistoryMeasurements summarize_export_history(
    const std::vector<DatedCount>& daily_counts) {
    ExportHistoryMeasurements measurements;
    if (daily_counts.empty()) {
        return measurements;
    }

    measurements.current_exports = daily_counts.back().count;
    measurements.prior_observation_count = daily_counts.size() - 1;
    if (daily_counts.size() < 2) {
        return measurements;
    }

    std::size_t prior_sum = 0;
    std::size_t prior_min = daily_counts.front().count;
    std::size_t prior_max = daily_counts.front().count;
    for (std::size_t i = 0; i + 1 < daily_counts.size(); ++i) {
        const auto count = daily_counts[i].count;
        prior_sum += count;
        prior_min = std::min(prior_min, count);
        prior_max = std::max(prior_max, count);
    }

    const auto prior_count = static_cast<double>(measurements.prior_observation_count);
    const auto prior_mean = static_cast<double>(prior_sum) / prior_count;
    measurements.prior_mean_exports = prior_mean;
    measurements.prior_min_exports = prior_min;
    measurements.prior_max_exports = prior_max;
    if (prior_mean > 0.0) {
        measurements.current_vs_prior_mean_ratio =
            static_cast<double>(measurements.current_exports) / prior_mean;
    }
    measurements.current_minus_prior_max =
        static_cast<std::int64_t>(measurements.current_exports) -
        static_cast<std::int64_t>(prior_max);
    return measurements;
}

std::vector<EntityFeatureSnapshot> extract_features(const EventStore& store) {
    const auto daily = count_daily_exports(store.events());

    std::vector<EntityFeatureSnapshot> snapshots;
    snapshots.reserve(daily.size());
    for (const auto& [entity_id, counts] : daily) {
        EntityFeatureSnapshot snapshot;
        snapshot.entity_id = entity_id;
        snapshot.daily_export_counts.reserve(counts.size());
        for (const auto& [day, count] : counts) {
            snapshot.daily_export_counts.push_back({day, count});
        }
        snapshot.window = {snapshot.daily_export_counts.front().day,
                           snapshot.daily_export_counts.back().day};
        snapshot.measurements = summarize_export_history(snapshot.daily_export_counts);
        for (std::size_t i = 0; i < store.size(); ++i) {
            if (store.events()[i].primary_entity_id == entity_id) {
                snapshot.supporting_event_indices.push_back(i);
            }
        }
        snapshots.push_back(std::move(snapshot));
    }
    return snapshots;
}

}  // namespace sift
