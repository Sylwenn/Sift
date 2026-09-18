//
// Created by rain on 17/09/26.
//
#include "src/analysis/analysis_runner.hpp"

#include <utility>

namespace sift {

std::vector<Measurement> standard_measurements(
    const EntityFeatureSnapshot& snapshot) {
    std::vector<Measurement> measurements;
    const auto& values = snapshot.measurements;
    measurements.push_back(
        {"current_exports", static_cast<double>(values.current_exports)});
    measurements.push_back({"prior_observation_count",
                            static_cast<double>(values.prior_observation_count)});
    if (values.prior_mean_exports) {
        measurements.push_back({"prior_mean_exports", *values.prior_mean_exports});
    }
    if (values.prior_min_exports) {
        measurements.push_back(
            {"prior_min_exports", static_cast<double>(*values.prior_min_exports)});
    }
    if (values.prior_max_exports) {
        measurements.push_back(
            {"prior_max_exports", static_cast<double>(*values.prior_max_exports)});
    }
    if (values.current_vs_prior_mean_ratio) {
        measurements.push_back({"current_vs_prior_mean_ratio",
                                *values.current_vs_prior_mean_ratio});
    }
    if (values.current_minus_prior_max) {
        measurements.push_back(
            {"current_minus_prior_max",
             static_cast<double>(*values.current_minus_prior_max)});
    }
    return measurements;
}

AnalysisResult run_native_analysis(const AnalysisInput& input) {
    AnalysisResult result;
    result.mode = AnalysisMode::native;
    result.items.reserve(input.entities.size());

    for (const auto& snapshot : input.entities) {
        AnalysisItem item;
        item.entity_id = snapshot.entity_id;
        item.window = snapshot.window;
        item.measurements = standard_measurements(snapshot);

        item.evidence.reserve(snapshot.supporting_event_indices.size());
        for (const auto index : snapshot.supporting_event_indices) {
            item.evidence.push_back({index});
        }

        result.items.push_back(std::move(item));
    }

    // Native finding selection and ranking require detector semantics. That
    // methodology is intentionally not implemented here.
    return result;
}

arrow::Result<AnalysisResult> run_analysis(AnalysisMode mode,
                                           const AnalysisInput& input,
                                           const AnalysisBackend* backend) {
    switch (mode) {
        case AnalysisMode::native:
            return run_native_analysis(input);
        case AnalysisMode::jev:
            if (backend == nullptr) {
                return arrow::Status::Invalid(
                    "Jev analysis backend is not available");
            }
            return backend->run(input);
    }
    return arrow::Status::Invalid("unsupported analysis mode");
}

}  // namespace sift
