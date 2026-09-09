#pragma once

#include <argparse/compat/detect.hpp>
#include <argparse/compat/string_view.hpp>
#include <cstdint>
#include <sstream>
#include <string>

#if ARGPARSE_HAS_STD_FORMAT
#include <format>
#endif

namespace argparse {

/**
 * @brief Categorized error codes returned by the argument parser.
 */
enum class error_code : uint8_t {
    success = 0,
    missing_required_argument,
    unexpected_positional,
    unknown_option,
    missing_value,
    invalid_value,
    mutually_exclusive_conflict,
    custom_validation_failed,
    choice_not_allowed
};

[[nodiscard]] inline ARGPARSE_CONSTEXPR14 string_view to_string_view(error_code code) noexcept {
    switch (code) {
    case error_code::success:
        return "success";
    case error_code::missing_required_argument:
        return "missing required argument";
    case error_code::unexpected_positional:
        return "unexpected positional argument";
    case error_code::unknown_option:
        return "unknown option";
    case error_code::missing_value:
        return "missing value for option";
    case error_code::invalid_value:
        return "invalid value conversion";
    case error_code::mutually_exclusive_conflict:
        return "mutually exclusive option conflict";
    case error_code::custom_validation_failed:
        return "custom validation constraint failed";
    case error_code::choice_not_allowed:
        return "value not in allowed choices";
    }
    return "unknown error";
}

/**
 * @brief Structured error information for parser diagnostic feedback.
 */
struct parse_error {
    error_code code{error_code::success};
    std::string argument_name{};
    std::string token{};
    std::string message{};

    constexpr parse_error() noexcept = default;

    parse_error(error_code c, string_view arg_name, string_view tok, std::string msg)
        : code(c), argument_name(arg_name.data(), arg_name.size()), token(tok.data(), tok.size()),
          message(std::move(msg)) {}

    parse_error(error_code c, string_view tok, std::string msg)
        : code(c), argument_name(""), token(tok.data(), tok.size()), message(std::move(msg)) {}

    [[nodiscard]] std::string to_string() const {
#if ARGPARSE_HAS_STD_FORMAT
        std::string result = std::format(
            "Error [{}]: {}", std::string_view(to_string_view(code).data(), to_string_view(code).size()), message);
        if (!argument_name.empty()) {
            result += std::format(" (argument: '{}')", argument_name);
        }
        if (!token.empty()) {
            result += std::format(" (token: '{}')", token);
        }
        return result;
#else
        std::ostringstream oss;
        oss << "Error [" << to_string_view(code) << "]: " << message;
        if (!argument_name.empty()) {
            oss << " (argument: '" << argument_name << "')";
        }
        if (!token.empty()) {
            oss << " (token: '" << token << "')";
        }
        return oss.str();
#endif
    }

    [[nodiscard]] bool operator==(const parse_error &other) const noexcept {
        return code == other.code && argument_name == other.argument_name && token == other.token;
    }
};

} // namespace argparse
