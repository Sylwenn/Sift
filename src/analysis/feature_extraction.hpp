//
// Created by rain on 17/09/26.
//

#ifndef SIFT_FEATURE_EXTRACTION_HPP
#define SIFT_FEATURE_EXTRACTION_HPP

#include <vector>

#include "src/analysis/analysis_types.hpp"
#include "src/storage/event_store.hpp"

namespace sift {

// Builds deterministic per-entity snapshots from the normalized events.
// It computes descriptive measurements only. It does not detect, score, rank,
// or apply thresholds.
std::vector<EntityFeatureSnapshot> extract_features(const EventStore& store);

// Computes descriptive latest-versus-prior export measurements for one daily
// series. The series is ordered by day, ascending. Exposed for testing.
ExportHistoryMeasurements summarize_export_history(
    const std::vector<DatedCount>& daily_counts);

}  // namespace sift

#endif  // SIFT_FEATURE_EXTRACTION_HPP
