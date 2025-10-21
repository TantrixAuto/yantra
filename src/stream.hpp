///PROTOTYPE_ENTER:SKIP
#pragma once
#include "util.hpp"
#include "error.hpp"
///PROTOTYPE_LEAVE:SKIP

// this class is stingified in the Generator class below (STREAMCLASS)
// any changes here must reflect there
struct Stream {
    std::istream& in;
    FilePos pos;
    char_t ch = 1;
    bool _eof = false;

    ///PROTOTYPE_ENTER:SKIP
    static inline char_t read(std::istream& in) {
        return in.peek();
    }
    ///PROTOTYPE_LEAVE:SKIP

    inline Stream(std::istream& i, const std::string_view& f) : in(i) {
        pos.file = f;
        pos.row = 1;
        pos.col = 1;
        ch = read(in);
    }

    inline const bool& eof() const {
        return _eof;
    }

    inline const char_t& peek() const {
        return ch;
    }

    inline void consume() {
        ///PROTOTYPE_ENTER:SKIP
        if(ch != read(in)) {
           throw GeneratorError(__LINE__, __FILE__, pos, "INVALID_INPUT");
        }
        ///PROTOTYPE_LEAVE:SKIP

        if (static_cast<int>(ch) == EOF) {
            _eof = true;
            return;
        }

        in.get();
        ch = read(in);
        pos.col++;
        if(ch == '\n') {
            pos.row++;
            pos.col = 1;
        }
    }
};

