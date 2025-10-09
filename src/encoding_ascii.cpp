/// @file encoding_ascii.cpp
/// @brief Contains all data structures and functions to read and manipulate ASCII strings

///PROTOTYPE_ENTER:SKIP
#include <stdint.h>
#include <cctype>
#include <iostream>
#include "encodings.hpp"
///PROTOTYPE_LEAVE:SKIP

///PROTOTYPE_ENTER:SKIP
namespace ascii {
///PROTOTYPE_LEAVE:SKIP

using char_t = char;

inline bool isSpace(const char_t& ch) {
    return std::isspace(ch);
}

inline bool isDigit(const char_t& ch) {
    return std::isdigit(ch);
}

inline bool isLetter(const char_t& ch) {
    return std::isalpha(ch);
}

inline bool isWord(const char_t& ch) {
    return std::isalnum(ch);
}
inline char_t read(std::istream& is) {
    char c0 = static_cast<char>(is.peek());
    return c0;
}

///PROTOTYPE_ENTER:SKIP
inline char_t castch(const uint32_t& ch) {
    char_t c0 = static_cast<char_t>(ch);
    return c0;
}
///PROTOTYPE_LEAVE:SKIP

///PROTOTYPE_ENTER:SKIP
} // namespace ascii
///PROTOTYPE_LEAVE:SKIP

///PROTOTYPE_ENTER:SKIP
bool Encodings::isAsciiLetterSubset(const uint32_t& ch1, const uint32_t& ch2) {
    if(ascii::isLetter(ascii::castch(ch1)) && ascii::isLetter(ascii::castch(ch2))) {
        return true;
    }
    return false;
}

///PROTOTYPE_LEAVE:SKIP
