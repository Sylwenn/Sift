//
// Created by rain on 17/09/26.
//
#include <chrono>
#include <string>
#include <utility>
#include <vector>

#include "src/analysis/analysis_runner.hpp"
#include "src/analysis/feature_extraction.hpp"
#include "src/storage/event_store.hpp"
#include "tests/test_support.hpp"

namespace {
sift::Event event_on(int day, std::string type, std::string entity) {
    const auto timestamp = std::chrono::sys_time<std::chrono::milliseconds>{
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::days{day})};
    return {timestamp, std::move(type), std::move(entity)};
}

class FakeBackend : public sift::AnalysisBackend {
public:
    arrow::Result<sift::AnalysisResult> run(const sift::AnalysisInput&) const override {
        sift::AnalysisResult result;
        result.mode = sift::AnalysisMode::jev;
        sift::AnalysisItem item;
        item.entity_id = "from-backend";
        result.items.push_back(std::move(item));
        return result;
    }
};
}

int main() {
    const std::vector<sift::Event> events{
        event_on(0, "login", "user-1"),
        event_on(2, "export", "user-1"),
        event_on(2, "export", "user-1"),
        event_on(1, "export", "user-2")};
    const sift::EventStore store{events};
    const auto snapshots = sift::extract_features(store);
    const sift::AnalysisInput input{store.events(), snapshots};

    const auto native = sift::run_native_analysis(input);
    CHECK(native.mode == sift::AnalysisMode::native);
    CHECK_EQ(native.items.size(), std::size_t{2});
    CHECK_EQ(native.items[0].entity_id, std::string{"user-1"});
    CHECK(native.items[0].judgments.empty());
    CHECK_EQ(native.items[0].evidence.size(), std::size_t{3});
    CHECK_EQ(native.items[0].evidence[0].event_index, std::size_t{0});
    CHECK(!native.items[0].measurements.empty());
    CHECK(native.items[1].judgments.empty());
    CHECK_EQ(native.items[1].entity_id, std::string{"user-2"});

    const auto dispatched =
        sift::run_analysis(sift::AnalysisMode::native, input, nullptr);
    CHECK(dispatched.ok());
    if (dispatched.ok()) {
        CHECK(dispatched->mode == sift::AnalysisMode::native);
        CHECK_EQ(dispatched->items.size(), std::size_t{2});
    }

    const auto jev_without_backend =
        sift::run_analysis(sift::AnalysisMode::jev, input, nullptr);
    CHECK(!jev_without_backend.ok());
    CHECK(jev_without_backend.status().message().find("not available") !=
          std::string::npos);

    const FakeBackend fake;
    const auto jev_with_backend =
        sift::run_analysis(sift::AnalysisMode::jev, input, &fake);
    CHECK(jev_with_backend.ok());
    if (jev_with_backend.ok()) {
        CHECK(jev_with_backend->mode == sift::AnalysisMode::jev);
        CHECK_EQ(jev_with_backend->items.size(), std::size_t{1});
        CHECK_EQ(jev_with_backend->items[0].entity_id, std::string{"from-backend"});
    }

    return sift_test::failures == 0 ? 0 : 1;
}
