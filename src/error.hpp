/// @file error.hpp
/// @brief This contains the only exception that gets thrown and handled in the entire app

#pragma once
#include "filepos.hpp"

class GeneratorError : public std::runtime_error {
    static inline std::string
    filename(const std::string& f) {
        std::filesystem::path p{f};
        return p.filename().string();
    }

    static inline std::string
    _fmt(const size_t& l, const std::string& f, const FilePos& p, const std::string& m) {
        auto ymsg = std::format("{}({:03d},{:03d}):{} ({}:{})", p.file, p.row, p.col, m, filename(f), l);
        return ymsg;
    }

public:
    const size_t line = 0;
    const std::string file;
    const FilePos pos;
    const std::string msg;

    template <typename ...ArgsT>
    inline GeneratorError(const size_t& l, const std::string& f, const FilePos& p, const std::format_string<ArgsT...>& m, ArgsT... args)
        : std::runtime_error{_fmt(l, f, p, fmt(m, args...))}, line{l}, file{filename(f)}, pos(p), msg{fmt(m, args...)}
    {}

    inline GeneratorError(const size_t& l, const std::string& f, const FilePos& p, const std::string& m)
        : std::runtime_error{_fmt(l, f, p, m)}, line{l}, file{filename(f)}, pos(p), msg{m}
    {}
};
