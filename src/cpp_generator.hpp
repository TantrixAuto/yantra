#pragma once
#include "grammar.hpp"

/// @brief true if output should be amalgamated
extern bool amalgamatedFile;

/// @brief entry function to generate cpp parser file
void generateGrammar(const yg::Grammar& g, const std::filesystem::path& of);

