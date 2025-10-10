/// @file encodings.hpp
/// @brief Functions for Yantra to access encodings.

#pragma once

struct Encodings {
    static bool isUnicodeLetterSubset(const uint32_t& ch1, const uint32_t& ch2);
    static bool isAsciiLetterSubset(const uint32_t& ch1, const uint32_t& ch2);
};
