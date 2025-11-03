#pragma once
#include "grammar_yg.hpp"

/// @brief entry function to generate cpp parser file
void generateGrammar(const yg::Grammar& g, const std::filesystem::path& of);

