/// @file filepos.hpp
/// @brief Represent location on file

#pragma once

/// @brief This class represents a location in a source file
struct FilePos {
    /// @brief The file this location points to
    std::string file;

    /// @brief The row number in the file this location points to
    size_t row = 1;

    /// @brief The col number in the row this location points to
    size_t col = 1;

    /// @brief return string representation of this location
    inline std::string str() const {
        return std::format("{}({:03d},{:03d})", file, row, col);
    }
};
