#pragma once

struct Logger {
    static std::ostream& olog();
};

template <typename ...ArgsT>
[[maybe_unused]]
static inline void log(const std::format_string<ArgsT...>& msg, ArgsT... args) {
    return std::println(Logger::olog(), msg, args...);
}
