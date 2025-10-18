#pragma once

/// @brief define char_t before including stream.hpp
using char_t = int;

#include "stream.hpp"
#include "grammar_yg.hpp"

/// @brief entry function to parse a .y file
void parseInput(yg::Grammar& g, Stream& is);
