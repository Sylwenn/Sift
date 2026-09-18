//
// Created by rain on 17/09/26.
//

#ifndef SIFT_JEV_ANALYSIS_HPP
#define SIFT_JEV_ANALYSIS_HPP

#include <nlohmann/json.hpp>

#include <string>

#include "src/analysis/analysis_runner.hpp"
#include "src/jev/jev_client.hpp"

namespace sift {

// Builds the bounded Jev state for one entity. It contains deterministic
// descriptive measurements only. Raw events are never serialized.
nlohmann::json build_jev_state(const EntityFeatureSnapshot& snapshot);

// The initial judgment is experimental. It characterizes precomputed facts.
// It is not native detection methodology and must not drive ranking.
std::string initial_jev_question_id();
JevChoiceQuestion initial_jev_question();

// Converts one Jev answer into a common analysis item. Probabilities and
// confidence stay separate, and provenance is retained.
AnalysisItem to_analysis_item(const EntityFeatureSnapshot& snapshot,
                              const JevChoiceAnswer& answer);

class JevAnalysisBackend : public AnalysisBackend {
public:
    explicit JevAnalysisBackend(JevClient client);

    arrow::Result<AnalysisResult> run(const AnalysisInput& input) const override;

private:
    JevClient client_;
};

}  // namespace sift

#endif  // SIFT_JEV_ANALYSIS_HPP
