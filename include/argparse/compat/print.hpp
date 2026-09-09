#pragma once

#include <argparse/compat/detect.hpp>
#include <argparse/compat/string_view.hpp>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

#if ARGPARSE_HAS_STD_PRINT
#include <ostream>
#include <print>

namespace argparse {
namespace compat {
using std::print;
using std::println;
} // namespace compat
using compat::print;
using compat::println;
} // namespace argparse
#elif ARGPARSE_HAS_STD_FORMAT
#include <format>
#include <iostream>

namespace argparse {
namespace compat {

template <typename... Args>
inline void print(std::format_string<Args...> fmt, Args &&...args) {
    std::cout << std::format(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void println(std::format_string<Args...> fmt, Args &&...args) {
    std::cout << std::format(fmt, std::forward<Args>(args)...) << '\n';
}

inline void println() {
    std::cout << '\n';
}

template <typename... Args>
inline void print(FILE *f, std::format_string<Args...> fmt, Args &&...args) {
    std::string s = std::format(fmt, std::forward<Args>(args)...);
    std::fwrite(s.data(), 1, s.size(), f);
}

template <typename... Args>
inline void println(FILE *f, std::format_string<Args...> fmt, Args &&...args) {
    std::string s = std::format(fmt, std::forward<Args>(args)...) + '\n';
    std::fwrite(s.data(), 1, s.size(), f);
}

inline void println(FILE *f) {
    std::fputc('\n', f);
}

template <typename... Args>
inline void print(std::ostream &os, std::format_string<Args...> fmt, Args &&...args) {
    os << std::format(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void println(std::ostream &os, std::format_string<Args...> fmt, Args &&...args) {
    os << std::format(fmt, std::forward<Args>(args)...) << '\n';
}

inline void println(std::ostream &os) {
    os << '\n';
}

} // namespace compat
using compat::print;
using compat::println;
} // namespace argparse
#else
namespace argparse {
namespace compat {

template <typename T>
inline void append_val(std::string &out, const T &val) {
    std::ostringstream oss;
    oss << val;
    out += oss.str();
}

inline void format_into(std::string &out, string_view fmt) {
    out.append(fmt.data(), fmt.size());
}

template <typename T, typename... Rest>
inline void format_into(std::string &out, string_view fmt, const T &first, const Rest &...rest) {
    size_t pos = fmt.find('{');
    if (pos != string_view::npos && pos + 1 < fmt.size() && fmt[pos + 1] == '}') {
        out.append(fmt.data(), pos);
        append_val(out, first);
        format_into(out, fmt.substr(pos + 2), rest...);
    } else {
        out.append(fmt.data(), fmt.size());
    }
}

inline void print() {}
inline void println() {
    std::cout << '\n';
}
inline void println(std::ostream &os) {
    os << '\n';
}

template <typename... Args>
inline void print(string_view fmt, const Args &...args) {
    std::string s;
    format_into(s, fmt, args...);
    std::cout << s;
}

template <typename... Args>
inline void println(string_view fmt, const Args &...args) {
    std::string s;
    format_into(s, fmt, args...);
    std::cout << s << '\n';
}

template <typename... Args>
inline void print(std::ostream &os, string_view fmt, const Args &...args) {
    std::string s;
    format_into(s, fmt, args...);
    os << s;
}

template <typename... Args>
inline void println(std::ostream &os, string_view fmt, const Args &...args) {
    std::string s;
    format_into(s, fmt, args...);
    os << s << '\n';
}

template <typename... Args>
inline void print(FILE *f, string_view fmt, const Args &...args) {
    std::string s;
    format_into(s, fmt, args...);
    std::fwrite(s.data(), 1, s.size(), f);
}

template <typename... Args>
inline void println(FILE *f, string_view fmt, const Args &...args) {
    std::string s;
    format_into(s, fmt, args...);
    s += '\n';
    std::fwrite(s.data(), 1, s.size(), f);
}

} // namespace compat
using compat::print;
using compat::println;
} // namespace argparse
#endif
