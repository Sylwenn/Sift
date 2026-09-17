//
// Created by rain on 07/08/26.
//

#ifndef SIFT_EVENT_STORE_HPP
#define SIFT_EVENT_STORE_HPP
#include <cstddef>
#include <span>
#include <vector>

#include "src/domain/event.hpp"

namespace sift {
class EventStore {
public:
    EventStore() = default;
    explicit EventStore(std::vector<Event> events);

    std::span<const Event> events() const noexcept;
    std::size_t size() const noexcept;
    bool empty() const noexcept;

private:
    std::vector<Event> events_;
};
}

#endif //SIFT_EVENT_STORE_HPP
