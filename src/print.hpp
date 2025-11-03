///PROTOTYPE_ENTER:SKIP
#pragma once
///PROTOTYPE_LEAVE:SKIP

#if __cplusplus < 202302L
namespace std {
template <typename ...ArgsT>
[[maybe_unused]]
static inline void print(std::ostream& os, const std::format_string<ArgsT...>& msg, ArgsT... args) {
    auto rv = std::format(msg, std::forward<ArgsT>(args)...);
    os << rv;
    std::cout.flush();
}

template <typename ...ArgsT>
[[maybe_unused]]
static inline void println(std::ostream& os, const std::format_string<ArgsT...>& msg, ArgsT... args) {
    auto rv = std::format(msg, std::forward<ArgsT>(args)...);
    os << rv << std::endl;
    std::cout.flush();
}

template <typename ...ArgsT>
[[maybe_unused]]
static inline void print(const std::format_string<ArgsT...>& msg, ArgsT... args) {
    return print(std::cout, msg, args...);
}

template <typename ...ArgsT>
[[maybe_unused]]
static inline void println(const std::format_string<ArgsT...>& msg, const ArgsT&... args) {
    return println(std::cout, msg, args...);
}
} // namespace std
#endif
