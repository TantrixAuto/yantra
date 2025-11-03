#pragma once

struct Logger {
    static auto olog() -> std::ostream&;
};

template <typename ...ArgsT>
[[maybe_unused]]
static inline void log(const std::format_string<ArgsT...>& msg, const ArgsT&... args) {
    return std::println(Logger::olog(), msg, args...);
}
