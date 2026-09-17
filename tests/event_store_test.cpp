//
// Created by rain on 07/08/26.
//
#include <chrono>
#include <string>
#include <utility>
#include <vector>

#include "src/storage/event_store.hpp"
#include "tests/test_support.hpp"

namespace {
sift::Event make_event(int64_t millis, std::string type, std::string entity) {
    return {std::chrono::sys_time<std::chrono::milliseconds>{
                std::chrono::milliseconds{millis}},
            std::move(type), std::move(entity)};
}
}

int main() {
    const sift::EventStore empty;
    CHECK(empty.empty());
    CHECK_EQ(empty.size(), std::size_t{0});
    CHECK(empty.events().empty());

    std::vector<sift::Event> raw{
        make_event(1000, "login", "user-1"),
        make_event(2000, "export", "user-2")};
    const sift::EventStore store{raw};
    CHECK(!store.empty());
    CHECK_EQ(store.size(), std::size_t{2});
    CHECK_EQ(store.events().size(), std::size_t{2});
    CHECK_EQ(store.events()[0].event_type, std::string{"login"});
    CHECK_EQ(store.events()[1].primary_entity_id, std::string{"user-2"});
    CHECK_EQ(store.events()[1].timestamp.time_since_epoch().count(), int64_t{2000});

    const sift::EventStore copy = store;
    CHECK_EQ(copy.size(), std::size_t{2});
    CHECK_EQ(copy.events()[0].event_type, std::string{"login"});

    std::vector<sift::Event> movable{make_event(3000, "login", "user-3")};
    const sift::EventStore moved{std::move(movable)};
    CHECK_EQ(moved.size(), std::size_t{1});
    CHECK_EQ(moved.events()[0].primary_entity_id, std::string{"user-3"});

    return sift_test::failures == 0 ? 0 : 1;
}
