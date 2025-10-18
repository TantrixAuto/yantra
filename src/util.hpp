#pragma once
#include "nsutil.hpp"
#include "print.hpp"

inline std::string getChString(const uint32_t& ch, const bool& md = false) {
    if(ch > 255){
        return std::format("0x{:04X}", ch);
    }

    if(isprint(static_cast<int>(ch)) == false) {
        return std::format("0x{:02X}", ch);
    }

    switch(ch) {
    case '\f':
        return std::format("'\\f'");
    case '\n':
        return std::format("'\\n'");
    case '\r':
        return std::format("'\\r'");
    case '\t':
        return std::format("'\\t'");
    case '\v':
        return std::format("'\\v'");
    case '\'':
        return std::format("'\\''");
    }

    if(md == true) {
        switch(ch) {
        case ';':
            return std::format("'#semi;'");
        case ':':
            return std::format("'#colon;'");
        case '.':
            return std::format("'#period;'");
        }
    }

    return std::format("'{:c}'", ch);
}

inline std::string getChString(const uint32_t& ch1, const uint32_t& ch2, const bool& md = false) {
    std::string s = getChString(ch1, md);
    if(ch2 != ch1) {
        s += "-";
        s += getChString(ch2, md);
    }
    return s;
}

template<typename T>
inline size_t zid(const T* t) {
    if(t == nullptr) {
        return 0;
    }
    return t->id;
}

static inline bool isHEX(const int& ch) {
    if ((ch >= '0') && (ch <= '9')) {
        return true;
    }
    if ((ch >= 'A') && (ch <= 'F')) {
        return true;
    }
    if ((ch >= 'a') && (ch <= 'f')) {
        return true;
    }
    return false;
}

static inline bool isDEC(const int& ch) {
    if ((ch >= '0') && (ch <= '9')) {
        return true;
    }
    return false;
}
