///PROTOTYPE_ENTER:SKIP
#pragma once
///PROTOTYPE_LEAVE:SKIP

template<typename... T>
[[maybe_unused]]
inline void unused(const T&...) {}

template<class... Ts>
struct overload : Ts... { using Ts::operator()...; };

struct NonCopyable{
    inline NonCopyable() = default;

    inline NonCopyable(const NonCopyable&) = delete;
    inline NonCopyable(NonCopyable&&) = delete;
    inline NonCopyable& operator=(const NonCopyable&) = delete;
    inline NonCopyable& operator=(NonCopyable&&) = delete;
};
