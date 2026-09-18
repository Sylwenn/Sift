//
// Created by rain on 17/09/26.
//
#include <chrono>
#include <string>
#include <utility>
#include <vector>

#include "src/analysis/feature_extraction.hpp"
#include "tests/test_support.hpp"

namespace {
sift::Event event_on(int day, std::string type, std::string entity) {
    const auto timestamp = std::chrono::sys_time<std::chrono::milliseconds>{
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::days{day})};
    return {timestamp, std::move(type), std::move(entity)};
}

sift::DatedCount dated(int day, std::size_t count) {
    return {std::chrono::sys_days{std::chrono::days{day}}, count};
}
}

int main() {
    const auto empty = sift::summarize_export_history({});
    CHECK_EQ(empty.current_exports, std::size_t{0});
    CHECK_EQ(empty.prior_observation_count, std::size_t{0});
    CHECK(!empty.prior_mean_exports.has_value());
    CHECK(!empty.current_vs_prior_mean_ratio.has_value());

    const auto single = sift::summarize_export_history({dated(0, 0)});
    CHECK_EQ(single.current_exports, std::size_t{0});
    CHECK_EQ(single.prior_observation_count, std::size_t{0});
    CHECK(!single.prior_mean_exports.has_value());
    CHECK(!single.current_minus_prior_max.has_value());

    const auto spike =
        sift::summarize_export_history({dated(0, 4), dated(1, 4), dated(2, 4),
                                        dated(3, 20)});
    CHECK_EQ(spike.current_exports, std::size_t{20});
    CHECK_EQ(spike.prior_observation_count, std::size_t{3});
    CHECK(spike.prior_mean_exports.has_value());
    CHECK_EQ(*spike.prior_mean_exports, 4.0);
    CHECK(spike.prior_min_exports.has_value());
    CHECK_EQ(*spike.prior_min_exports, std::size_t{4});
    CHECK(spike.prior_max_exports.has_value());
    CHECK_EQ(*spike.prior_max_exports, std::size_t{4});
    CHECK(spike.current_vs_prior_mean_ratio.has_value());
    CHECK_EQ(*spike.current_vs_prior_mean_ratio, 5.0);
    CHECK(spike.current_minus_prior_max.has_value());
    CHECK_EQ(*spike.current_minus_prior_max, std::int64_t{16});

    const auto zero_prior = sift::summarize_export_history({dated(0, 0), dated(1, 5)});
    CHECK(!zero_prior.current_vs_prior_mean_ratio.has_value());
    CHECK(zero_prior.prior_mean_exports.has_value());
    CHECK_EQ(*zero_prior.prior_mean_exports, 0.0);
    CHECK(zero_prior.current_minus_prior_max.has_value());
    CHECK_EQ(*zero_prior.current_minus_prior_max, std::int64_t{5});

    const std::vector<sift::Event> events{
        event_on(0, "login", "user-1"),
        event_on(2, "export", "user-1"),
        event_on(2, "export", "user-1"),
        event_on(1, "export", "user-2")};
    const sift::EventStore store{events};
    const auto snapshots = sift::extract_features(store);

    CHECK_EQ(snapshots.size(), std::size_t{2});
    const auto& user1 = snapshots[0];
    CHECK_EQ(user1.entity_id, std::string{"user-1"});
    CHECK_EQ(user1.daily_export_counts.size(), std::size_t{3});
    CHECK_EQ(user1.daily_export_counts[0].count, std::size_t{0});
    CHECK_EQ(user1.daily_export_counts[1].count, std::size_t{0});
    CHECK_EQ(user1.daily_export_counts[2].count, std::size_t{2});
    CHECK(user1.window.start == std::chrono::sys_days{std::chrono::days{0}});
    CHECK(user1.window.end == std::chrono::sys_days{std::chrono::days{2}});
    CHECK_EQ(user1.measurements.current_exports, std::size_t{2});
    CHECK_EQ(user1.measurements.prior_observation_count, std::size_t{2});
    CHECK_EQ(user1.supporting_event_indices.size(), std::size_t{3});
    CHECK_EQ(user1.supporting_event_indices[0], std::size_t{0});
    CHECK_EQ(user1.supporting_event_indices[2], std::size_t{2});

    const auto& user2 = snapshots[1];
    CHECK_EQ(user2.entity_id, std::string{"user-2"});
    CHECK_EQ(user2.daily_export_counts.size(), std::size_t{1});
    CHECK_EQ(user2.measurements.prior_observation_count, std::size_t{0});
    CHECK_EQ(user2.supporting_event_indices.size(), std::size_t{1});
    CHECK_EQ(user2.supporting_event_indices[0], std::size_t{3});

    const sift::EventStore empty_store;
    CHECK(sift::extract_features(empty_store).empty());

    return sift_test::failures == 0 ? 0 : 1;
}
