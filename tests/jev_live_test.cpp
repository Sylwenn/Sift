//
// Created by rain on 17/09/26.
//
// Opt-in live Jev integration test.
//
// It makes a real, paid API call and is never registered under normal ctest.
// Enable it with -DSIFT_RUN_LIVE_JEV_TESTS=ON and set TYPESAFE_API_KEY.
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "src/analysis/feature_extraction.hpp"
#include "src/jev/jev_analysis.hpp"
#include "tests/test_support.hpp"

int main() {
    if (std::getenv("SIFT_RUN_LIVE_JEV_TESTS") == nullptr) {
        std::cout << "jev_live: skipped (SIFT_RUN_LIVE_JEV_TESTS is not set)\n";
        return 0;
    }

    auto api_key = sift::read_typesafe_api_key();
    if (!api_key.ok()) {
        std::cerr << "jev_live: " << api_key.status().ToString() << '\n';
        return 1;
    }

    sift::EntityFeatureSnapshot snapshot;
    snapshot.entity_id = "user-142";
    snapshot.window.start = std::chrono::sys_days{std::chrono::days{0}};
    snapshot.window.end = std::chrono::sys_days{std::chrono::days{3}};
    snapshot.daily_export_counts = {
        {std::chrono::sys_days{std::chrono::days{0}}, 4},
        {std::chrono::sys_days{std::chrono::days{1}}, 4},
        {std::chrono::sys_days{std::chrono::days{2}}, 4},
        {std::chrono::sys_days{std::chrono::days{3}}, 20}};
    snapshot.measurements =
        sift::summarize_export_history(snapshot.daily_export_counts);

    sift::JevClientOptions options;
    options.api_key = *api_key;
    options.model = "jev-1.13.0";
    if (const char* base = std::getenv("SIFT_TYPESAFE_BASE_URL");
        base != nullptr && *base != '\0') {
        options.base_url = base;
    }

    const sift::CurlJevTransport transport;
    const sift::JevClient client{options, transport};
    const auto answer = client.ask_choice(sift::build_jev_state(snapshot),
                                          sift::initial_jev_question_id(),
                                          sift::initial_jev_question());
    CHECK(answer.ok());
    if (!answer.ok()) {
        std::cerr << "jev_live: " << answer.status().ToString() << '\n';
        return 1;
    }

    CHECK_EQ(answer->probabilities.size(), std::size_t{3});
    CHECK(answer->probabilities.count(answer->selected_value) == 1);
    std::cout << "jev_live: selected=" << answer->selected_value
              << " model=" << answer->model << '\n';
    return sift_test::failures == 0 ? 0 : 1;
}
