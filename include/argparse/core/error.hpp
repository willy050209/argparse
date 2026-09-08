#pragma once

#include <cstdint>
#include <format>
#include <string>
#include <string_view>

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

[[nodiscard]] constexpr std::string_view to_string_view(error_code code) noexcept {
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

    constexpr parse_error(error_code c, std::string_view arg_name, std::string_view tok, std::string msg)
        : code(c), argument_name(arg_name), token(tok), message(std::move(msg)) {}

    constexpr parse_error(error_code c, std::string_view tok, std::string msg)
        : code(c), argument_name(""), token(tok), message(std::move(msg)) {}

    [[nodiscard]] std::string to_string() const {
        std::string result = std::format("Error [{}]: {}", to_string_view(code), message);
        if (!argument_name.empty()) {
            result += std::format(" (argument: '{}')", argument_name);
        }
        if (!token.empty()) {
            result += std::format(" (token: '{}')", token);
        }
        return result;
    }

    [[nodiscard]] bool operator==(const parse_error& other) const noexcept {
        return code == other.code && argument_name == other.argument_name && token == other.token;
    }
};

} // namespace argparse
