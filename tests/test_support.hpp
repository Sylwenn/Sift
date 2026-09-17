//
// Created by rain on 07/08/26.
//

#ifndef SIFT_TEST_SUPPORT_HPP
#define SIFT_TEST_SUPPORT_HPP
#include <iostream>

namespace sift_test {
inline int failures = 0;
}

#define CHECK(condition)                                                   \
    do {                                                                   \
        if (!(condition)) {                                                \
            std::cerr << __FILE__ << ':' << __LINE__                       \
                      << ": CHECK failed: " #condition << '\n';            \
            ++sift_test::failures;                                         \
        }                                                                  \
    } while (false)

#define CHECK_EQ(actual, expected)                                         \
    do {                                                                   \
        const auto& check_actual = (actual);                               \
        const auto& check_expected = (expected);                           \
        if (!(check_actual == check_expected)) {                           \
            std::cerr << __FILE__ << ':' << __LINE__                       \
                      << ": CHECK_EQ failed: " #actual " == " #expected    \
                      << "\n  actual:   " << check_actual                  \
                      << "\n  expected: " << check_expected << '\n';       \
            ++sift_test::failures;                                         \
        }                                                                  \
    } while (false)

#endif //SIFT_TEST_SUPPORT_HPP
