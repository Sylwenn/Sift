//
// Created by rain on 07/08/26.
//
#include "src/storage/event_store.hpp"

#include <utility>

namespace sift {
EventStore::EventStore(std::vector<Event> events) : events_(std::move(events)) {}

std::span<const Event> EventStore::events() const noexcept {
    return events_;
}

std::size_t EventStore::size() const noexcept {
    return events_.size();
}

bool EventStore::empty() const noexcept {
    return events_.empty();
}
}
