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

//C: libstdc++ ships std::println(std::ostream&, format_string, args...) as a
//C: non-standard extension; the C++23 standard only defines stdout/FILE*
//C: overloads. Every std::println(os, ...) call site in this codebase
//C: relies on the extension. libc++ (Apple Clang) and MSVC's STL don't
//C: provide it, so supply it here, guarded so it doesn't redefine
//C: libstdc++'s own version where that extension already exists.
#ifndef __GLIBCXX__
namespace std {
template <typename ...ArgsT>
void println(std::ostream& os, format_string<ArgsT...> fmt, ArgsT&&... args) {
    os << std::format(fmt, std::forward<ArgsT>(args)...) << '\n';
}
}
#endif
