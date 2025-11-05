#pragma once

struct Options {
    /// @brief true to generate an amalgamated .cpp file instead of separate .hpp and .cpp files
    bool amalgamatedFile = false;

    /// @brief true to insert #line statements for codeblocks in the generated file
    bool genLines = true;

    /// @brief filename where the AST is logged in markdown format
    std::string gfilename;

    /// @brief enable lexer logging
    bool enableLexerLogging = false;
};

auto opts() -> const Options&;
