//
// Created by rain on 07/08/26.
//

#ifndef SIFT_EVENT_HPP
#define SIFT_EVENT_HPP
#include <chrono>
#include <string>
namespace sift {
    struct Event {
        std::chrono::sys_time<std::chrono::milliseconds> timestamp;
        std::string event_type;
        std::string primary_entity_id;
    };
}


#endif //SIFT_EVENT_HPP
