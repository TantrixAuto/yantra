/// @file encoding_ascii.cpp
/// @brief Contains all data structures and functions to read and manipulate ASCII strings

///PROTOTYPE_ENTER:SKIP
#include <cstdint>
#include <cctype>
#include <iostream>
#include "encodings.hpp"
///PROTOTYPE_LEAVE:SKIP

///PROTOTYPE_ENTER:SKIP
namespace ascii {
///PROTOTYPE_LEAVE:SKIP

using char_t = char;

inline auto isSpace(const char_t& ch) -> bool {
    return (std::isspace(ch) != 0);
}

inline auto isDigit(const char_t& ch) -> bool {
    return (std::isdigit(ch) != 0);
}

inline auto isLetter(const char_t& ch) -> bool {
    return (std::isalpha(ch) != 0);
}

inline auto isWord(const char_t& ch) -> bool {
    return (std::isalnum(ch) != 0);
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
