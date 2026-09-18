//
// Created by rain on 17/09/26.
//

#ifndef SIFT_ANALYSIS_TYPES_HPP
#define SIFT_ANALYSIS_TYPES_HPP

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "src/config/app_config.hpp"
#include "src/domain/event.hpp"

namespace sift {

// A closed calendar-day interval observed in the input.
struct TimeWindow {
    std::chrono::sys_days start;
    std::chrono::sys_days end;
};

struct DatedCount {
    std::chrono::sys_days day;
    std::size_t count;
};

// Descriptive arithmetic only. These values are not scores, severities,
// probabilities, or priorities, and they carry no thresholds.
struct ExportHistoryMeasurements {
    std::size_t current_exports = 0;
    std::size_t prior_observation_count = 0;
    std::optional<double> prior_mean_exports;
    std::optional<std::size_t> prior_min_exports;
    std::optional<std::size_t> prior_max_exports;
    std::optional<double> current_vs_prior_mean_ratio;
    std::optional<std::int64_t> current_minus_prior_max;
};

struct EntityFeatureSnapshot {
    std::string entity_id;
    TimeWindow window;
    std::vector<DatedCount> daily_export_counts;
    ExportHistoryMeasurements measurements;
    std::vector<std::size_t> supporting_event_indices;
};

struct AnalysisInput {
    std::span<const Event> events;
    std::vector<EntityFeatureSnapshot> entities;
};

struct Measurement {
    std::string name;
    double value;
};

struct EvidenceReference {
    std::size_t event_index;
};

struct ModelProvenance {
    std::string provider;
    std::string model;
    std::string request_id;
};

struct TokenUsage {
    std::int64_t input_tokens = 0;
    std::int64_t output_tokens = 0;
};

struct ProbabilisticJudgment {
    std::string judgment_id;
    std::string selected_value;
    std::map<std::string, double> probabilities;
    std::optional<double> confidence;
    ModelProvenance provenance;
};

// An item is a measured and possibly judged entity. It is deliberately not
// called a Finding until selection semantics are defined.
struct AnalysisItem {
    std::string entity_id;
    TimeWindow window;
    std::vector<Measurement> measurements;
    std::vector<EvidenceReference> evidence;
    std::vector<ProbabilisticJudgment> judgments;
};

struct AnalysisResult {
    AnalysisMode mode = AnalysisMode::native;
    std::vector<AnalysisItem> items;
    TokenUsage usage;
};

}  // namespace sift

#endif  // SIFT_ANALYSIS_TYPES_HPP
