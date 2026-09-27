#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <format>
#include <unordered_map>
#include <vector>
#include <variant>
#include <set>
#include <print>
#include <unordered_set>
#include <functional>
#include <ranges>
#include <assert.h>
#include <algorithm>

#if !defined(__GLIBCXX__) && !defined(_MSVC_STL_VERSION)
namespace std {
template <typename ...ArgsT>
void println(std::ostream& os, format_string<ArgsT...> fmt, ArgsT&&... args) {
    os << std::format(fmt, std::forward<ArgsT>(args)...) << '\n';
}
}
#endif
