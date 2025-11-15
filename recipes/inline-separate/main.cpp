#include "grammar.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if(argc < 2) {
        std::cout << argv[0] << " <expr>" << std::endl;
        return 0;
    }
    YantraModule mod(argv[0]);
    mod.readString(argv[1], "in");
    mod.walk_Compiler();
    return 0;
}
