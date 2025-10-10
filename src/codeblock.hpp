/// @file codeblock.hpp
/// @brief Implements a codeblock class

#pragma once

#include "filepos.hpp"

/// @brief Represents a code block, typically semantic actions associated with a rule
struct CodeBlock {
    /// @brief Position of this codeblock in original .y file
    FilePos pos;

    /// @brief true if there is a position ssociated with this codeblock
    bool hasPos = false;

    /// @brief the text of the codeblock
    std::string code;

    /// @brief true if this codeblock is not empty
    inline bool hasCode() const {
        return code.size() > 0;
    }

    /// @brief set text in this codeblock without a position
    inline void setCode(const std::string& c) {
        code = c;
    }

    /// @brief set text in this codeblock with a position
    inline void setCode(const FilePos& p, const std::string& c) {
        pos = p;
        code = c;
        hasPos = true;
    }
};
