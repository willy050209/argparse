#pragma once

#include <argparse/core/error.hpp>
#include <argparse/core/traits.hpp>

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <format>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace argparse {

namespace detail {

[[nodiscard]] constexpr bool iequals(std::string_view lhs, std::string_view rhs) noexcept {
    return std::ranges::equal(lhs, rhs, [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
    });
}

template <typename IntT>
[[nodiscard]] std::expected<IntT, parse_error> parse_integral(std::string_view sv) noexcept {
    if (sv.empty()) {
        return std::unexpected(parse_error{error_code::invalid_value, sv, "Empty string cannot be parsed as integer"});
    }

    int base = 10;
    std::string_view num_part = sv;
    bool negative = false;

    if (num_part.starts_with('-')) {
        if constexpr (std::is_unsigned_v<IntT>) {
            return std::unexpected(parse_error{error_code::invalid_value, sv, "Cannot parse negative number into unsigned type"});
        }
        negative = true;
        num_part.remove_prefix(1);
    } else if (num_part.starts_with('+')) {
        num_part.remove_prefix(1);
    }

    if (num_part.starts_with("0x") || num_part.starts_with("0X")) {
        base = 16;
        num_part.remove_prefix(2);
    } else if (num_part.starts_with("0b") || num_part.starts_with("0B")) {
        base = 2;
        num_part.remove_prefix(2);
    }

    if (num_part.empty()) {
        return std::unexpected(parse_error{error_code::invalid_value, sv, "Invalid integer format"});
    }

    IntT val{};
    auto [ptr, ec] = std::from_chars(num_part.data(), num_part.data() + num_part.size(), val, base);
    if (ec != std::errc{} || ptr != num_part.data() + num_part.size()) {
        if (ec == std::errc::result_out_of_range) {
            return std::unexpected(parse_error{error_code::invalid_value, sv, "Integer value out of range"});
        }
        return std::unexpected(parse_error{error_code::invalid_value, sv, "Invalid integer literal"});
    }

    if constexpr (std::is_signed_v<IntT>) {
        if (negative) {
            val = static_cast<IntT>(-val);
        }
    }

    return val;
}

} // namespace detail

// ==========================================
// Bool Parser
// ==========================================
template <>
struct value_parser<bool> {
    [[nodiscard]] static std::expected<bool, parse_error> parse(std::string_view sv) noexcept {
        if (detail::iequals(sv, "true") || detail::iequals(sv, "1") ||
            detail::iequals(sv, "yes") || detail::iequals(sv, "on") ||
            detail::iequals(sv, "t") || detail::iequals(sv, "y")) {
            return true;
        }
        if (detail::iequals(sv, "false") || detail::iequals(sv, "0") ||
            detail::iequals(sv, "no") || detail::iequals(sv, "off") ||
            detail::iequals(sv, "f") || detail::iequals(sv, "n")) {
            return false;
        }
        return std::unexpected(parse_error{error_code::invalid_value, sv, "Cannot parse value as boolean"});
    }
};

// ==========================================
// Fixed-width Integer Parsers
// ==========================================
template <typename IntT>
    requires std::integral<IntT> && (!std::same_as<IntT, bool>)
struct value_parser<IntT> {
    [[nodiscard]] static std::expected<IntT, parse_error> parse(std::string_view sv) noexcept {
        return detail::parse_integral<IntT>(sv);
    }
};

// ==========================================
// Floating-point Parsers (float, double)
// ==========================================
template <std::floating_point FloatT>
struct value_parser<FloatT> {
    [[nodiscard]] static std::expected<FloatT, parse_error> parse(std::string_view sv) noexcept {
        if (sv.empty()) {
            return std::unexpected(parse_error{error_code::invalid_value, sv, "Empty string cannot be parsed as float"});
        }
        FloatT val{};
        auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), val);
        if (ec != std::errc{} || ptr != sv.data() + sv.size()) {
            if (ec == std::errc::result_out_of_range) {
                return std::unexpected(parse_error{error_code::invalid_value, sv, "Floating point value out of range"});
            }
            return std::unexpected(parse_error{error_code::invalid_value, sv, "Invalid floating-point literal"});
        }
        return val;
    }
};

// ==========================================
// String & String View Parsers
// ==========================================
template <>
struct value_parser<std::string> {
    [[nodiscard]] static std::expected<std::string, parse_error> parse(std::string_view sv) {
        return std::string(sv);
    }
};

template <>
struct value_parser<std::string_view> {
    [[nodiscard]] static std::expected<std::string_view, parse_error> parse(std::string_view sv) noexcept {
        return sv;
    }
};

// ==========================================
// Filesystem Path Parser
// ==========================================
template <>
struct value_parser<std::filesystem::path> {
    [[nodiscard]] static std::expected<std::filesystem::path, parse_error> parse(std::string_view sv) {
        return std::filesystem::path(sv);
    }
};

// ==========================================
// Character Parser
// ==========================================
template <>
struct value_parser<char> {
    [[nodiscard]] static std::expected<char, parse_error> parse(std::string_view sv) noexcept {
        if (sv.size() != 1) {
            return std::unexpected(parse_error{error_code::invalid_value, sv, "Expected a single character"});
        }
        return sv.front();
    }
};

// ==========================================
// Vector Container Parser
// ==========================================
template <parsable T>
struct value_parser<std::vector<T>> {
    [[nodiscard]] static std::expected<std::vector<T>, parse_error> parse(std::string_view sv) {
        std::vector<T> result;
        if (sv.empty()) {
            return result;
        }

        // Support comma/semicolon delimited items: "item1,item2,item3"
        size_t start = 0;
        while (start < sv.size()) {
            size_t end = sv.find_first_of(",;", start);
            if (end == std::string_view::npos) {
                end = sv.size();
            }
            std::string_view part = sv.substr(start, end - start);
            // Trim leading/trailing whitespace
            while (!part.empty() && std::isspace(static_cast<unsigned char>(part.front()))) {
                part.remove_prefix(1);
            }
            while (!part.empty() && std::isspace(static_cast<unsigned char>(part.back()))) {
                part.remove_suffix(1);
            }

            if (!part.empty()) {
                auto parsed_item = value_parser<T>::parse(part);
                if (!parsed_item) {
                    return std::unexpected(parsed_item.error());
                }
                result.push_back(std::move(*parsed_item));
            }

            start = end + 1;
        }

        return result;
    }
};

} // namespace argparse
