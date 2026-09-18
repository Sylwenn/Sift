//
// Created by rain on 17/09/26.
//

#ifndef SIFT_ANALYSIS_RUNNER_HPP
#define SIFT_ANALYSIS_RUNNER_HPP

#include <arrow/result.h>

#include "src/analysis/analysis_types.hpp"

namespace sift {

// Narrow seam for optional analysis backends such as Jev. It keeps external
// model infrastructure out of shared code and lets tests substitute a fake.
class AnalysisBackend {
public:
    virtual ~AnalysisBackend() = default;
    virtual arrow::Result<AnalysisResult> run(const AnalysisInput& input) const = 0;
};

// Produces one measurement-only item per entity. It does not select, score, or
// rank findings.
AnalysisResult run_native_analysis(const AnalysisInput& input);

// Dispatches on the selected mode. The backend is only required for jev.
arrow::Result<AnalysisResult> run_analysis(AnalysisMode mode,
                                           const AnalysisInput& input,
                                           const AnalysisBackend* backend);

}  // namespace sift

#endif  // SIFT_ANALYSIS_RUNNER_HPP
