//
// Created by rain on 17/09/26.
//
#include <chrono>
#include <string>
#include <utility>
#include <vector>

#include "src/analysis/feature_extraction.hpp"
#include "src/jev/jev_analysis.hpp"
#include "src/storage/event_store.hpp"
#include "tests/test_support.hpp"

namespace {
class FakeTransport : public sift::JevTransport {
public:
    mutable int calls = 0;

    arrow::Result<sift::JevHttpResponse> send(
        const sift::JevHttpRequest&) const override {
        ++calls;
        sift::JevHttpResponse response;
        response.status_code = 200;
        response.headers["x-typesafe-request-id"] = "req-1";
        response.body =
            "{\"model\":\"jev-1.13.0\",\"answers\":{\"change_characterization\":{"
            "\"type\":\"choice\",\"choice\":\"clear_departure_from_prior_observations\","
            "\"probabilities\":{\"consistent_with_prior_observations\":0.05,"
            "\"clear_departure_from_prior_observations\":0.85,"
            "\"insufficient_prior_context\":0.10},\"confidence\":0.78}},"
            "\"usage\":{\"input_tokens\":10,\"output_tokens\":2}}";
        return response;
    }
};

sift::Event export_on(int day, std::string entity) {
    const auto timestamp = std::chrono::sys_time<std::chrono::milliseconds>{
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::days{day})};
    return {timestamp, "export", std::move(entity)};
}
}

int main() {
    std::vector<sift::Event> events;
    for (int i = 0; i < 4; ++i) {
        events.push_back(export_on(0, "user-142"));
        events.push_back(export_on(1, "user-142"));
    }
    for (int i = 0; i < 20; ++i) {
        events.push_back(export_on(2, "user-142"));
    }
    events.push_back(export_on(0, "user-007"));

    const sift::EventStore store{events};
    const auto snapshots = sift::extract_features(store);
    CHECK_EQ(snapshots.size(), std::size_t{2});

    const auto& single_day = snapshots[0];
    CHECK_EQ(single_day.entity_id, std::string{"user-007"});
    const auto state = sift::build_jev_state(single_day);
    CHECK_EQ(state["entity_id"].get<std::string>(), std::string{"user-007"});
    CHECK_EQ(state["observation_window"]["start"].get<std::string>(),
             std::string{"1970-01-01"});
    CHECK_EQ(state["observation_window"]["end"].get<std::string>(),
             std::string{"1970-01-01"});
    CHECK_EQ(state["measurements"]["current_exports"].get<std::size_t>(),
             std::size_t{1});
    CHECK_EQ(state["measurements"]["prior_observation_count"].get<std::size_t>(),
             std::size_t{0});
    CHECK(!state["measurements"].contains("prior_mean_exports"));
    CHECK(!state["measurements"].contains("current_vs_prior_mean_ratio"));
    // Raw events are not serialized into the Jev state.
    CHECK(state.find("events") == state.end());

    const auto& spiky = snapshots[1];
    CHECK_EQ(spiky.entity_id, std::string{"user-142"});
    const auto spiky_state = sift::build_jev_state(spiky);
    CHECK_EQ(spiky_state["measurements"]["current_exports"].get<std::size_t>(),
             std::size_t{20});
    CHECK_EQ(
        spiky_state["measurements"]["prior_observation_count"].get<std::size_t>(),
        std::size_t{2});
    CHECK_EQ(spiky_state["measurements"]["prior_max_exports"].get<std::size_t>(),
             std::size_t{4});
    CHECK_EQ(
        spiky_state["measurements"]["current_minus_prior_max"].get<std::int64_t>(),
        std::int64_t{16});
    CHECK(spiky_state.find("events") == spiky_state.end());

    const auto question = sift::initial_jev_question();
    CHECK_EQ(sift::initial_jev_question_id(), std::string{"change_characterization"});
    CHECK_EQ(question.criteria.size(), std::size_t{3});
    CHECK(question.criteria.count("clear_departure_from_prior_observations") == 1);

    FakeTransport transport;
    sift::JevClientOptions options;
    options.api_key = "test-key";
    options.base_url = "https://example.test";
    sift::JevClient client{options, transport};
    const sift::JevAnalysisBackend backend{std::move(client)};

    const sift::AnalysisInput input{store.events(), snapshots};
    const auto result = backend.run(input);
    CHECK(result.ok());
    if (result.ok()) {
        CHECK(result->mode == sift::AnalysisMode::jev);
        CHECK_EQ(result->items.size(), std::size_t{2});
        CHECK_EQ(result->usage.input_tokens, std::int64_t{20});
        CHECK_EQ(result->usage.output_tokens, std::int64_t{4});

        const auto& item = result->items[1];
        CHECK_EQ(item.entity_id, std::string{"user-142"});
        CHECK(!item.measurements.empty());
        CHECK_EQ(item.evidence.size(), std::size_t{28});
        CHECK_EQ(item.judgments.size(), std::size_t{1});
        const auto& judgment = item.judgments[0];
        CHECK_EQ(judgment.judgment_id, std::string{"change_characterization"});
        CHECK_EQ(judgment.selected_value,
                 std::string{"clear_departure_from_prior_observations"});
        CHECK_EQ(judgment.probabilities.size(), std::size_t{3});
        CHECK(judgment.confidence.has_value());
        CHECK_EQ(*judgment.confidence, 0.78);
        CHECK_EQ(judgment.provenance.provider, std::string{"typesafe"});
        CHECK_EQ(judgment.provenance.model, std::string{"jev-1.13.0"});
        CHECK_EQ(judgment.provenance.request_id, std::string{"req-1"});
    }
    CHECK_EQ(transport.calls, 2);

    return sift_test::failures == 0 ? 0 : 1;
}
