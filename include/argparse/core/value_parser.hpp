#pragma once

#include <algorithm>
#include <argparse/compat/detect.hpp>
#include <argparse/compat/expected.hpp>
#include <argparse/compat/string_view.hpp>
#include <argparse/compat/traits.hpp>
#include <argparse/core/error.hpp>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

#if defined(__has_include)
#if __has_include(<charconv>) && ARGPARSE_CPLUSPLUS >= 201703L
#include <charconv>
#define ARGPARSE_HAS_CHARCONV_HEADER 1
#else
#define ARGPARSE_HAS_CHARCONV_HEADER 0
#endif
#if __has_include(<filesystem>) && ARGPARSE_CPLUSPLUS >= 201703L
#include <filesystem>
#define ARGPARSE_HAS_STD_FILESYSTEM 1
#else
#define ARGPARSE_HAS_STD_FILESYSTEM 0
#endif
#else
#define ARGPARSE_HAS_CHARCONV_HEADER 0
#define ARGPARSE_HAS_STD_FILESYSTEM 0
#endif

namespace argparse {

namespace detail {

[[nodiscard]] inline bool iequals(string_view lhs, string_view rhs) noexcept {
    if (lhs.size() != rhs.size())
        return false;
    for (size_t i = 0; i < lhs.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(lhs[i])) != std::tolower(static_cast<unsigned char>(rhs[i]))) {
            return false;
        }
    }
    return true;
}

template <typename IntT>
inline IntT apply_sign(IntT val, bool negative, std::true_type) noexcept {
    return negative ? static_cast<IntT>(-val) : val;
}

template <typename IntT>
inline IntT apply_sign(IntT val, bool /*negative*/, std::false_type) noexcept {
    return val;
}

template <typename IntT>
[[nodiscard]] expected<IntT, parse_error> parse_integral(string_view sv) noexcept {
    if (sv.empty()) {
        return unexpected<parse_error>(
            parse_error{error_code::invalid_value, sv, "Empty string cannot be parsed as integer"});
    }

    int base = 10;
    string_view num_part = sv;
    bool negative = false;

    if (starts_with(num_part, '-')) {
        if (std::is_unsigned<IntT>::value) {
            return unexpected<parse_error>(
                parse_error{error_code::invalid_value, sv, "Cannot parse negative number into unsigned type"});
        }
        negative = true;
        num_part.remove_prefix(1);
    } else if (starts_with(num_part, '+')) {
        num_part.remove_prefix(1);
    }

    if (starts_with(num_part, "0x") || starts_with(num_part, "0X")) {
        base = 16;
        num_part.remove_prefix(2);
    } else if (starts_with(num_part, "0b") || starts_with(num_part, "0B")) {
        base = 2;
        num_part.remove_prefix(2);
    }

    if (num_part.empty()) {
        return unexpected<parse_error>(parse_error{error_code::invalid_value, sv, "Invalid integer format"});
    }

#if ARGPARSE_HAS_CHARCONV_HEADER
    IntT val{};
    auto [ptr, ec] = std::from_chars(num_part.data(), num_part.data() + num_part.size(), val, base);
    if (ec != std::errc{} || ptr != num_part.data() + num_part.size()) {
        if (ec == std::errc::result_out_of_range) {
            return unexpected<parse_error>(parse_error{error_code::invalid_value, sv, "Integer value out of range"});
        }
        return unexpected<parse_error>(parse_error{error_code::invalid_value, sv, "Invalid integer literal"});
    }

    val = apply_sign(val, negative, std::is_signed<IntT>{});
    return val;
#else
    std::string s(num_part.data(), num_part.size());
    char *endptr = nullptr;
    if (std::is_signed<IntT>::value) {
        long long res = std::strtoll(s.c_str(), &endptr, base);
        if (endptr != s.c_str() + s.size()) {
            return unexpected<parse_error>(parse_error{error_code::invalid_value, sv, "Invalid integer literal"});
        }
        if (negative)
            res = -res;
        return static_cast<IntT>(res);
    } else {
        unsigned long long res = std::strtoull(s.c_str(), &endptr, base);
        if (endptr != s.c_str() + s.size()) {
            return unexpected<parse_error>(parse_error{error_code::invalid_value, sv, "Invalid integer literal"});
        }
        return static_cast<IntT>(res);
    }
#endif
}

} // namespace detail

// ==========================================
// Bool Parser
// ==========================================
template <>
struct value_parser<bool> {
    [[nodiscard]] static expected<bool, parse_error> parse(string_view sv) noexcept {
        if (detail::iequals(sv, "true") || detail::iequals(sv, "1") || detail::iequals(sv, "yes") ||
            detail::iequals(sv, "on") || detail::iequals(sv, "t") || detail::iequals(sv, "y")) {
            return true;
        }
        if (detail::iequals(sv, "false") || detail::iequals(sv, "0") || detail::iequals(sv, "no") ||
            detail::iequals(sv, "off") || detail::iequals(sv, "f") || detail::iequals(sv, "n")) {
            return false;
        }
        return unexpected<parse_error>(parse_error{error_code::invalid_value, sv, "Cannot parse value as boolean"});
    }
};

// ==========================================
// Integer Parsers
// ==========================================
template <typename IntT>
struct value_parser<IntT, enable_if_t<std::is_integral<IntT>::value && !std::is_same<IntT, bool>::value>> {
    [[nodiscard]] static expected<IntT, parse_error> parse(string_view sv) noexcept {
        return detail::parse_integral<IntT>(sv);
    }
};

// ==========================================
// Floating-point Parsers
// ==========================================
template <typename FloatT>
struct value_parser<FloatT, enable_if_t<std::is_floating_point<FloatT>::value>> {
    [[nodiscard]] static expected<FloatT, parse_error> parse(string_view sv) noexcept {
        if (sv.empty()) {
            return unexpected<parse_error>(
                parse_error{error_code::invalid_value, sv, "Empty string cannot be parsed as float"});
        }
#if ARGPARSE_HAS_CHARCONV_HEADER && defined(__cpp_lib_to_chars) && (__cpp_lib_to_chars >= 201611L)
        FloatT val{};
        auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), val);
        if (ec != std::errc{} || ptr != sv.data() + sv.size()) {
            return unexpected<parse_error>(
                parse_error{error_code::invalid_value, sv, "Invalid floating-point literal"});
        }
        return val;
#else
        std::string s(sv.data(), sv.size());
        char *endptr = nullptr;
        double val = std::strtod(s.c_str(), &endptr);
        if (endptr != s.c_str() + s.size()) {
            return unexpected<parse_error>(
                parse_error{error_code::invalid_value, sv, "Invalid floating-point literal"});
        }
        return static_cast<FloatT>(val);
#endif
    }
};

// ==========================================
// String & String View Parsers
// ==========================================
template <>
struct value_parser<std::string> {
    [[nodiscard]] static expected<std::string, parse_error> parse(string_view sv) {
        return std::string(sv.data(), sv.size());
    }
};

template <>
struct value_parser<string_view> {
    [[nodiscard]] static expected<string_view, parse_error> parse(string_view sv) noexcept { return sv; }
};

// ==========================================
// Filesystem Path Parser
// ==========================================
#if ARGPARSE_HAS_STD_FILESYSTEM
template <>
struct value_parser<std::filesystem::path> {
    [[nodiscard]] static expected<std::filesystem::path, parse_error> parse(string_view sv) {
        return std::filesystem::path(std::string_view(sv.data(), sv.size()));
    }
};
#endif

// ==========================================
// Character Parser
// ==========================================
template <>
struct value_parser<char> {
    [[nodiscard]] static expected<char, parse_error> parse(string_view sv) noexcept {
        if (sv.size() != 1) {
            return unexpected<parse_error>(parse_error{error_code::invalid_value, sv, "Expected a single character"});
        }
        return sv.front();
    }
};

// ==========================================
// Vector Container Parser
// ==========================================
template <typename T>
struct value_parser<std::vector<T>> {
    [[nodiscard]] static expected<std::vector<T>, parse_error> parse(string_view sv) {
        std::vector<T> result;
        if (sv.empty()) {
            return result;
        }

        size_t start = 0;
        while (start < sv.size()) {
            size_t end = sv.find_first_of(",;", start);
            if (end == string_view::npos) {
                end = sv.size();
            }
            string_view part = sv.substr(start, end - start);
            while (!part.empty() && std::isspace(static_cast<unsigned char>(part.front()))) {
                part.remove_prefix(1);
            }
            while (!part.empty() && std::isspace(static_cast<unsigned char>(part.back()))) {
                part.remove_suffix(1);
            }

            if (!part.empty()) {
                auto parsed_item = value_parser<T>::parse(part);
                if (!parsed_item) {
                    return unexpected<parse_error>(parsed_item.error());
                }
                result.push_back(std::move(*parsed_item));
            }

            start = end + 1;
        }

        return result;
    }
};

} // namespace argparse
