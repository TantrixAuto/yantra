///PROTOTYPE_ENTER:SKIP
#pragma once
///PROTOTYPE_LEAVE:SKIP

#ifndef UNUSED_DECLARED
template<typename... T>
[[maybe_unused]]
inline void unused(const T&...) {} // NOLINT(hicpp-named-parameter,readability-named-parameter)
#endif

#ifndef OVERLOAD_DECLARED
template<class... Ts>
struct overload : Ts... { using Ts::operator()...; };
#endif

struct NonCopyable{
    inline ~NonCopyable() = default;
    inline NonCopyable() = default;

    inline NonCopyable(const NonCopyable&) = delete;
    inline NonCopyable(NonCopyable&&) = delete;
    inline auto operator=(const NonCopyable& src) -> NonCopyable& = delete;
    inline auto operator=(NonCopyable&& src) -> NonCopyable& = delete;
};
