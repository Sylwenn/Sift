//
// Created by rain on 17/09/26.
//
#include "src/jev/jev_analysis.hpp"

#include <cstdio>
#include <utility>

namespace sift {
namespace {

std::string format_date(std::chrono::sys_days day) {
    const std::chrono::year_month_day calendar{day};
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d",
                  static_cast<int>(calendar.year()),
                  static_cast<unsigned>(calendar.month()),
                  static_cast<unsigned>(calendar.day()));
    return std::string{buffer};
}

}  // namespace

nlohmann::json build_jev_state(const EntityFeatureSnapshot& snapshot) {
    nlohmann::json state;
    state["entity_id"] = snapshot.entity_id;
    state["observation_window"]["start"] = format_date(snapshot.window.start);
    state["observation_window"]["end"] = format_date(snapshot.window.end);

    const auto& values = snapshot.measurements;
    auto& measurements = state["measurements"];
    measurements["current_exports"] = values.current_exports;
    measurements["prior_observation_count"] = values.prior_observation_count;
    if (values.prior_mean_exports) {
        measurements["prior_mean_exports"] = *values.prior_mean_exports;
    }
    if (values.prior_min_exports) {
        measurements["prior_min_exports"] = *values.prior_min_exports;
    }
    if (values.prior_max_exports) {
        measurements["prior_max_exports"] = *values.prior_max_exports;
    }
    if (values.current_vs_prior_mean_ratio) {
        measurements["current_vs_prior_mean_ratio"] =
            *values.current_vs_prior_mean_ratio;
    }
    if (values.current_minus_prior_max) {
        measurements["current_minus_prior_max"] = *values.current_minus_prior_max;
    }
    return state;
}

std::string initial_jev_question_id() { return "change_characterization"; }

JevChoiceQuestion initial_jev_question() {
    JevChoiceQuestion question;
    question.instructions = {
        {"question",
         "Which description best characterizes the current export activity "
         "using the supplied precomputed measurements?"},
        {"scope",
         nlohmann::json::array(
             {"Use only the supplied descriptive facts.",
              "Do not perform additional arithmetic.",
              "Do not infer maliciousness, intent, compromise, severity, or "
              "review priority."})}};
    question.criteria = {
        {"consistent_with_prior_observations",
         "The supplied facts do not show a clear departure from prior "
         "observations."},
        {"clear_departure_from_prior_observations",
         "The supplied facts show a clear descriptive departure from prior "
         "observations."},
        {"insufficient_prior_context",
         "The supplied prior observations are insufficient to characterize the "
         "change."}};
    return question;
}

AnalysisItem to_analysis_item(const EntityFeatureSnapshot& snapshot,
                              const JevChoiceAnswer& answer) {
    AnalysisItem item;
    item.entity_id = snapshot.entity_id;
    item.window = snapshot.window;
    item.measurements = standard_measurements(snapshot);
    item.evidence.reserve(snapshot.supporting_event_indices.size());
    for (const auto index : snapshot.supporting_event_indices) {
        item.evidence.push_back({index});
    }

    ProbabilisticJudgment judgment;
    judgment.judgment_id = initial_jev_question_id();
    judgment.selected_value = answer.selected_value;
    judgment.probabilities = answer.probabilities;
    judgment.confidence = answer.confidence;
    judgment.provenance = {"typesafe", answer.model, answer.request_id};
    item.judgments.push_back(std::move(judgment));
    return item;
}

JevAnalysisBackend::JevAnalysisBackend(JevClient client)
    : client_(std::move(client)) {}

arrow::Result<AnalysisResult> JevAnalysisBackend::run(
    const AnalysisInput& input) const {
    AnalysisResult result;
    result.mode = AnalysisMode::jev;
    result.items.reserve(input.entities.size());

    const auto question = initial_jev_question();
    const auto question_id = initial_jev_question_id();
    for (const auto& snapshot : input.entities) {
        auto answer = client_.ask_choice(build_jev_state(snapshot), question_id,
                                         question);
        if (!answer.ok()) {
            return answer.status();
        }
        result.usage.input_tokens += answer->usage.input_tokens;
        result.usage.output_tokens += answer->usage.output_tokens;
        result.items.push_back(to_analysis_item(snapshot, *answer));
    }
    return result;
}

}  // namespace sift
