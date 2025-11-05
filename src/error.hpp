/// @file error.hpp
/// @brief This contains the only exception that gets thrown and handled in the entire app

#pragma once
#include "filepos.hpp"

/// @brief this function formats an error message
static inline auto
formatError(const FilePos& p, const std::string& msg) -> std::string {
    auto ymsg = std::format("{}:{}:{}: error: {}", p.file, p.row, p.col, msg);
    return ymsg;
}

template <typename ...ArgsT>
static inline void
printError(const FilePos& p, const std::format_string<ArgsT...>& msg, ArgsT... args){
    auto ymsg = std::format(msg, std::forward<ArgsT>(args)...);
    ymsg = formatError(p, ymsg);
    std::println("{}", ymsg);
}

/// @brief This exception is thrown when a grammar error is encountered
class GeneratorError : public std::runtime_error {
    static inline auto
    filename(const std::string& f) -> std::string{
        std::filesystem::path p{f};
        return p.filename().string();
    }

    template <typename ...ArgsT>
    static inline auto
    fmt(const std::format_string<ArgsT...>& msg, ArgsT... args) -> std::string{
        return std::format(msg, std::forward<ArgsT>(args)...);
    }

    static inline auto
    _fmt(const size_t& l, const std::string& f, const FilePos& p, const std::string& m) -> std::string {
        auto ymsg = std::format("{} ({}:{})", m, filename(f), l);
        return formatError(p, ymsg);
    }

public:
    const size_t line = 0;
    const std::string file;
    const FilePos pos;
    const std::string msg;

    template <typename ...ArgsT>
    inline GeneratorError(const size_t& l, const std::string& f, const FilePos& p, const std::format_string<ArgsT...>& m, const ArgsT&... args)
        : std::runtime_error{_fmt(l, f, p, fmt(m, args...))}, line{l}, file{filename(f)}, pos(p), msg{fmt(m, args...)}
    {}

    inline GeneratorError(const size_t& l, const std::string& f, const FilePos& p, const std::string& m)
        : std::runtime_error{_fmt(l, f, p, m)}, line{l}, file{filename(f)}, pos(p), msg{m}
    {}
};
