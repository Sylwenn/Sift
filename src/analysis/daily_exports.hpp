//
// Created by rain on 07/08/26.
//

#ifndef SIFT_DAILY_EXPORTS_HPP
#define SIFT_DAILY_EXPORTS_HPP
#include <chrono>
#include <cstddef>
#include <map>
#include <span>
#include <string>

#include "src/domain/event.hpp"

namespace sift {
using DailyExportCounts =
    std::map<std::string, std::map<std::chrono::sys_days, std::size_t>>;

DailyExportCounts count_daily_exports(std::span<const Event> events);
}

#endif //SIFT_DAILY_EXPORTS_HPP
