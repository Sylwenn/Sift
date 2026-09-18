//
// Created by rain on 17/09/26.
//
#include <chrono>
#include <string>
#include <utility>
#include <vector>

#include "src/analysis/feature_extraction.hpp"
#include "src/reporting/text_reporter.hpp"
#include "src/storage/event_store.hpp"
#include "tests/test_support.hpp"

namespace {
sift::Event event_on(int day, std::string type, std::string entity) {
    const auto timestamp = std::chrono::sys_time<std::chrono::milliseconds>{
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::days{day})};
    return {timestamp, std::move(type), std::move(entity)};
}
}

int main() {
    std::vector<sift::Event> events;
    for (int day = 0; day < 2; ++day) {
        for (int i = 0; i < 4; ++i) {
            events.push_back(event_on(day, "export", "user-142"));
            events.push_back(event_on(day, "export", "user-007"));
        }
    }
    const sift::EventStore store{events};
    const auto snapshots = sift::extract_features(store);

    const auto native =
        sift::render_native_report("occurred_at: timestamp[ms]", snapshots,
                                   store.size());
    const std::string expected_native =
        "occurred_at: timestamp[ms]\n"
        "events: 16\n"
        "user-007 daily_exports: 4 4\n"
        "user-142 daily_exports: 4 4\n";
    CHECK_EQ(native, expected_native);

    const auto empty_native =
        sift::render_native_report("s", std::vector<sift::EntityFeatureSnapshot>{}, 0);
    CHECK_EQ(empty_native, std::string{"s\nevents: 0\n"});

    sift::AnalysisResult jev;
    jev.mode = sift::AnalysisMode::jev;
    sift::AnalysisItem item;
    item.entity_id = "user-142";
    sift::ProbabilisticJudgment judgment;
    judgment.judgment_id = "change_characterization";
    judgment.selected_value = "clear_departure_from_prior_observations";
    judgment.probabilities = {
        {"consistent_with_prior_observations", 0.05},
        {"clear_departure_from_prior_observations", 0.85},
        {"insufficient_prior_context", 0.1}};
    judgment.confidence = 0.78;
    judgment.provenance = {"typesafe", "jev-1.13.0", "req-1"};
    item.judgments.push_back(judgment);
    jev.items.push_back(item);

    const auto jev_text = sift::render_jev_report("s", jev, 16);
    const std::string expected_jev =
        "s\n"
        "events: 16\n"
        "user-142 change_characterization: clear_departure_from_prior_observations"
        " p(clear_departure_from_prior_observations)=0.85"
        " p(consistent_with_prior_observations)=0.05"
        " p(insufficient_prior_context)=0.1 confidence=0.78\n";
    CHECK_EQ(jev_text, expected_jev);

    return sift_test::failures == 0 ? 0 : 1;
}
