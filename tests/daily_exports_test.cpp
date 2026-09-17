//
// Created by rain on 07/08/26.
//
#include <chrono>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "src/analysis/daily_exports.hpp"
#include "tests/test_support.hpp"

namespace {
sift::Event event_on(int day, std::string type, std::string entity) {
    const auto timestamp = std::chrono::sys_time<std::chrono::milliseconds>{
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::days{day})};
    return {timestamp, std::move(type), std::move(entity)};
}

std::chrono::sys_days day(int value) {
    return std::chrono::sys_days{std::chrono::days{value}};
}
}

int main() {
    const std::vector<sift::Event> none;
    const auto empty_counts = sift::count_daily_exports(none);
    CHECK(empty_counts.empty());

    const std::vector<sift::Event> events{
        event_on(0, "login", "user-1"),
        event_on(2, "export", "user-1"),
        event_on(2, "export", "user-1"),
        event_on(1, "export", "user-2")};
    const auto counts = sift::count_daily_exports(events);

    CHECK_EQ(counts.size(), std::size_t{2});
    CHECK_EQ(counts.begin()->first, std::string{"user-1"});
    CHECK_EQ(std::next(counts.begin())->first, std::string{"user-2"});

    const auto& user1 = counts.at("user-1");
    CHECK_EQ(user1.size(), std::size_t{3});
    CHECK_EQ(user1.at(day(0)), std::size_t{0});
    CHECK_EQ(user1.at(day(1)), std::size_t{0});
    CHECK_EQ(user1.at(day(2)), std::size_t{2});

    const auto& user2 = counts.at("user-2");
    CHECK_EQ(user2.size(), std::size_t{1});
    CHECK_EQ(user2.at(day(1)), std::size_t{1});

    const std::vector<sift::Event> single{event_on(5, "login", "user-9")};
    const auto single_counts = sift::count_daily_exports(single);
    CHECK_EQ(single_counts.size(), std::size_t{1});
    CHECK_EQ(single_counts.at("user-9").size(), std::size_t{1});
    CHECK_EQ(single_counts.at("user-9").at(day(5)), std::size_t{0});

    return sift_test::failures == 0 ? 0 : 1;
}
