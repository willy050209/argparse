// ============================================================================
// argparse: High-performance, type-safe, zero-overhead C++23 argument parser
// https://github.com/modern-cpp/argparse
// Distributed under the MIT License.
// ============================================================================
#pragma once

#include <algorithm>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <format>
#include <functional>
#include <iostream>
#include <optional>
#include <ranges>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// --- Begin: core/error.hpp ---
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
// --- End: core/error.hpp ---


// --- Begin: core/traits.hpp ---
namespace argparse {

/**
 * @brief Primary template for type parsing customization point.
 * Specializations must provide:
 * static std::expected<T, parse_error> parse(std::string_view sv);
 */
template <typename T, typename = void>
struct value_parser;

/**
 * @brief Concept testing whether T has a valid value_parser specialization.
 */
template <typename T>
concept parsable = requires(std::string_view sv) {
    { value_parser<T>::parse(sv) } -> std::same_as<std::expected<T, parse_error>>;
};

/**
 * @brief Concept for a boolean validator predicate on type T.
 */
template <typename F, typename T>
concept boolean_validator_for = requires(F&& f, const T& val) {
    { std::forward<F>(f)(val) } -> std::convertible_to<bool>;
};

/**
 * @brief Concept for an expected-returning validator on type T.
 */
template <typename F, typename T>
concept result_validator_for = requires(F&& f, const T& val) {
    { std::forward<F>(f)(val) } -> std::same_as<std::expected<void, std::string>>;
};

/**
 * @brief General validator concept accepting either boolean or expected return.
 */
template <typename F, typename T>
concept validator_for = boolean_validator_for<F, T> || result_validator_for<F, T>;

/**
 * @brief Type traits for detecting container types (e.g. std::vector<T>).
 */
template <typename T>
struct is_vector : std::false_type {};

template <typename T, typename Alloc>
struct is_vector<std::vector<T, Alloc>> : std::true_type {};

template <typename T>
inline constexpr bool is_vector_v = is_vector<T>::value;

} // namespace argparse
// --- End: core/traits.hpp ---


// --- Begin: core/value_parser.hpp ---
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
// --- End: core/value_parser.hpp ---


// --- Begin: core/token.hpp ---
namespace argparse {

enum class token_type {
    long_option,       // e.g. --verbose or --port=8080
    short_option,      // e.g. -v or -p8080 or chained -xvf
    positional,        // e.g. filename.txt
    options_delimiter  // explicitly --
};

struct token {
    token_type type{token_type::positional};
    std::string_view raw{};
    std::string_view name{};
    std::optional<std::string_view> inline_value{};
};

} // namespace argparse
// --- End: core/token.hpp ---


// --- Begin: config/action.hpp ---
namespace argparse {

enum class action : uint8_t {
    store,        // Store single value (default for options that take arguments)
    store_true,   // Store boolean true if flag is present, false otherwise
    store_false,  // Store boolean false if flag is present, true otherwise
    append,       // Append each occurrence to a list/vector
    count         // Count number of times the flag appears (e.g. -vvv)
};

} // namespace argparse
// --- End: config/action.hpp ---


// --- Begin: config/argument.hpp ---
namespace argparse {

class argument {
public:
    using validator_fn = std::function<std::expected<void, std::string>(std::string_view)>;

    explicit argument(std::string_view name, std::string_view short_name = "")
        : m_name(name), m_short_name(short_name) {
        // Determine if positional: doesn't start with '-'
        m_positional = !m_name.starts_with('-');
        if (m_positional) {
            m_metavar = m_name;
        } else {
            // Default metavar derived from name
            std::string_view clean = m_name;
            while (clean.starts_with('-')) {
                clean.remove_prefix(1);
            }
            m_metavar = clean;
            for (char& c : m_metavar) {
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            }
        }
    }

    argument& help(std::string_view help_text) {
        m_help = std::string(help_text);
        return *this;
    }

    argument& metavar(std::string_view mv) {
        m_metavar = std::string(mv);
        return *this;
    }

    argument& required(bool req = true) {
        m_required = req;
        return *this;
    }

    argument& flag() {
        m_action = action::store_true;
        m_default_value = "false";
        m_implicit_value = "true";
        return *this;
    }

    argument& count() {
        m_action = action::count;
        m_default_value = "0";
        return *this;
    }

    argument& append() {
        m_action = action::append;
        return *this;
    }

    template <typename T>
    argument& default_value(const T& val) {
        if constexpr (std::same_as<T, std::string>) {
            m_default_value = val;
        } else if constexpr (std::same_as<T, std::string_view> || std::same_as<T, const char*>) {
            m_default_value = std::string(val);
        } else if constexpr (std::same_as<T, bool>) {
            m_default_value = val ? "true" : "false";
        } else {
            m_default_value = std::format("{}", val);
        }
        return *this;
    }

    template <typename T>
    argument& implicit_value(const T& val) {
        if constexpr (std::same_as<T, std::string>) {
            m_implicit_value = val;
        } else if constexpr (std::same_as<T, std::string_view> || std::same_as<T, const char*>) {
            m_implicit_value = std::string(val);
        } else if constexpr (std::same_as<T, bool>) {
            m_implicit_value = val ? "true" : "false";
        } else {
            m_implicit_value = std::format("{}", val);
        }
        return *this;
    }

    template <std::convertible_to<std::string_view>... Args>
    argument& choices(Args&&... chs) {
        (m_choices.emplace_back(std::forward<Args>(chs)), ...);
        return *this;
    }

    argument& choices(std::span<const std::string_view> chs) {
        for (auto sv : chs) {
            m_choices.emplace_back(sv);
        }
        return *this;
    }

    argument& choices(const std::vector<std::string>& chs) {
        m_choices = chs;
        return *this;
    }

    template <typename T, typename F>
        requires parsable<T> && validator_for<F, T>
    argument& validator(F&& func, std::string error_msg = "Constraint validation failed") {
        m_validators.push_back([f = std::forward<F>(func), msg = std::move(error_msg)](std::string_view raw) -> std::expected<void, std::string> {
            auto parsed = value_parser<T>::parse(raw);
            if (!parsed) {
                return std::unexpected(parsed.error().message);
            }
            if constexpr (result_validator_for<F, T>) {
                return f(*parsed);
            } else {
                if (!f(*parsed)) {
                    return std::unexpected(msg);
                }
                return {};
            }
        });
        return *this;
    }

    template <typename F>
        requires (!parsable<F>)
    argument& validator(F&& func, std::string error_msg = "Validation failed") {
        m_validators.push_back([f = std::forward<F>(func), msg = std::move(error_msg)](std::string_view raw) -> std::expected<void, std::string> {
            if (!f(raw)) {
                return std::unexpected(msg);
            }
            return {};
        });
        return *this;
    }

    argument& group_id(size_t gid) noexcept {
        m_group_id = gid;
        return *this;
    }

    // Accessors
    [[nodiscard]] const std::string& name() const noexcept { return m_name; }
    [[nodiscard]] const std::string& short_name() const noexcept { return m_short_name; }
    [[nodiscard]] const std::string& help() const noexcept { return m_help; }
    [[nodiscard]] const std::string& metavar() const noexcept { return m_metavar; }
    [[nodiscard]] action get_action() const noexcept { return m_action; }
    [[nodiscard]] bool is_required() const noexcept { return m_required; }
    [[nodiscard]] bool is_positional() const noexcept { return m_positional; }
    [[nodiscard]] bool is_flag() const noexcept { return m_action == action::store_true || m_action == action::store_false; }
    [[nodiscard]] bool takes_value() const noexcept { return m_action == action::store || m_action == action::append; }
    [[nodiscard]] const std::optional<std::string>& default_value() const noexcept { return m_default_value; }
    [[nodiscard]] const std::optional<std::string>& implicit_value() const noexcept { return m_implicit_value; }
    [[nodiscard]] const std::vector<std::string>& get_choices() const noexcept { return m_choices; }
    [[nodiscard]] const std::vector<validator_fn>& validators() const noexcept { return m_validators; }
    [[nodiscard]] std::optional<size_t> get_group_id() const noexcept { return m_group_id; }

    [[nodiscard]] bool matches(std::string_view opt) const noexcept {
        return opt == m_name || (!m_short_name.empty() && opt == m_short_name);
    }

private:
    std::string m_name;
    std::string m_short_name;
    std::string m_help;
    std::string m_metavar;
    action m_action{action::store};
    bool m_required{false};
    bool m_positional{false};
    std::optional<std::string> m_default_value;
    std::optional<std::string> m_implicit_value;
    std::vector<std::string> m_choices;
    std::vector<validator_fn> m_validators;
    std::optional<size_t> m_group_id;
};

} // namespace argparse
// --- End: config/argument.hpp ---


// --- Begin: config/argument_group.hpp ---
namespace argparse {

// Forward declaration
class argument_parser;

class argument_group {
public:
    argument_group(size_t id, argument_parser& parent, bool mutually_exclusive = false, bool required = false)
        : m_id(id), m_parent(&parent), m_mutually_exclusive(mutually_exclusive), m_required(required) {}

    argument& add_argument(std::string_view name, std::string_view short_name = "");

    [[nodiscard]] size_t id() const noexcept { return m_id; }
    [[nodiscard]] bool is_mutually_exclusive() const noexcept { return m_mutually_exclusive; }
    [[nodiscard]] bool is_required() const noexcept { return m_required; }
    [[nodiscard]] const std::vector<std::string>& argument_names() const noexcept { return m_argument_names; }

    void register_argument_name(std::string_view name) {
        m_argument_names.emplace_back(name);
    }

private:
    size_t m_id;
    argument_parser* m_parent;
    bool m_mutually_exclusive{false};
    bool m_required{false};
    std::vector<std::string> m_argument_names;
};

} // namespace argparse
// --- End: config/argument_group.hpp ---


// --- Begin: engine/parse_result.hpp ---
namespace argparse {

/**
 * @brief Immutable, type-safe argument parsing result container.
 */
class parse_result {
public:
    parse_result() = default;

    /**
     * @brief Check if an option or flag was provided on the command line or has a default.
     */
    [[nodiscard]] bool has(std::string_view name) const noexcept {
        auto canonical = resolve_canonical_name(name);
        auto it = m_values.find(std::string(canonical));
        if (it == m_values.end() || it->second.empty()) {
            return false;
        }
        // If it's a flag with value "false", has() returns false if user didn't explicitly specify it
        if (it->second.size() == 1 && it->second.front() == "false") {
            return m_explicitly_present.contains(std::string(canonical));
        }
        return true;
    }

    /**
     * @brief Check if argument was explicitly provided by the user (ignoring defaults).
     */
    [[nodiscard]] bool is_explicit(std::string_view name) const noexcept {
        auto canonical = resolve_canonical_name(name);
        return m_explicitly_present.contains(std::string(canonical));
    }

    /**
     * @brief Number of occurrences of a flag or option.
     */
    [[nodiscard]] size_t count(std::string_view name) const noexcept {
        auto canonical = resolve_canonical_name(name);
        auto it = m_values.find(std::string(canonical));
        if (it == m_values.end()) {
            return 0;
        }
        // If it's a counter action stored as integer string
        if (it->second.size() == 1) {
            auto parsed = value_parser<size_t>::parse(it->second.front());
            if (parsed) {
                return *parsed;
            }
        }
        return it->second.size();
    }

    /**
     * @brief Get raw string value of an argument.
     */
    [[nodiscard]] std::optional<std::string_view> get_raw(std::string_view name) const noexcept {
        auto canonical = resolve_canonical_name(name);
        auto it = m_values.find(std::string(canonical));
        if (it == m_values.end() || it->second.empty()) {
            return std::nullopt;
        }
        return it->second.back();
    }

    /**
     * @brief Get all raw values associated with an argument (e.g. for append action).
     */
    [[nodiscard]] std::vector<std::string_view> get_raw_list(std::string_view name) const {
        std::vector<std::string_view> result;
        auto canonical = resolve_canonical_name(name);
        auto it = m_values.find(std::string(canonical));
        if (it != m_values.end()) {
            result.reserve(it->second.size());
            for (const auto& s : it->second) {
                result.emplace_back(s);
            }
        }
        return result;
    }

    /**
     * @brief Try to get and convert value to type T without throwing exceptions.
     */
    template <parsable T>
    [[nodiscard]] std::optional<T> try_get(std::string_view name) const noexcept {
        auto canonical = resolve_canonical_name(name);
        auto it = m_values.find(std::string(canonical));
        if (it == m_values.end() || it->second.empty()) {
            return std::nullopt;
        }

        if constexpr (is_vector_v<T>) {
            // For vector types, if multiple values were recorded, reconstruct
            using ItemT = typename T::value_type;
            T vec;
            for (const auto& raw_val : it->second) {
                // If the single item itself is comma-separated, parse with value_parser<std::vector<ItemT>>
                auto parsed_sub = value_parser<T>::parse(raw_val);
                if (parsed_sub) {
                    for (auto&& item : *parsed_sub) {
                        vec.push_back(std::move(item));
                    }
                } else {
                    auto single_item = value_parser<ItemT>::parse(raw_val);
                    if (single_item) {
                        vec.push_back(std::move(*single_item));
                    }
                }
            }
            return vec;
        } else {
            auto parsed = value_parser<T>::parse(it->second.back());
            if (parsed) {
                return *parsed;
            }
            return std::nullopt;
        }
    }

    /**
     * @brief Strongly-typed access to argument value.
     */
    template <parsable T>
    [[nodiscard]] T get(std::string_view name) const {
        auto val = try_get<T>(name);
        if (!val) {
            throw std::runtime_error(std::format("Argument '{}' not found or failed to convert to target type", name));
        }
        return std::move(*val);
    }

    /**
     * @brief Strongly-typed access with fallback default value.
     */
    template <parsable T>
    [[nodiscard]] T get_or(std::string_view name, T fallback) const noexcept {
        auto val = try_get<T>(name);
        if (val) {
            return std::move(*val);
        }
        return fallback;
    }

    /**
     * @brief Get positional arguments in order of appearance.
     */
    [[nodiscard]] const std::vector<std::string>& positionals() const noexcept {
        return m_positionals;
    }

    // Builder methods (used by parsing engine)
    void set_value(std::string_view canonical_name, std::string value, bool is_explicit = true) {
        std::string name_str(canonical_name);
        m_values[name_str] = {std::move(value)};
        if (is_explicit) {
            m_explicitly_present.insert(name_str);
        }
    }

    void append_value(std::string_view canonical_name, std::string value, bool is_explicit = true) {
        std::string name_str(canonical_name);
        m_values[name_str].push_back(std::move(value));
        if (is_explicit) {
            m_explicitly_present.insert(name_str);
        }
    }

    void add_positional(std::string value) {
        m_positionals.push_back(std::move(value));
    }

    void register_alias(std::string_view alias, std::string_view canonical_name) {
        m_alias_map[std::string(alias)] = std::string(canonical_name);
    }

private:
    [[nodiscard]] std::string_view resolve_canonical_name(std::string_view name) const noexcept {
        auto it = m_alias_map.find(std::string(name));
        if (it != m_alias_map.end()) {
            return it->second;
        }
        return name;
    }

    std::unordered_map<std::string, std::vector<std::string>> m_values;
    std::unordered_map<std::string, std::string> m_alias_map;
    std::unordered_set<std::string> m_explicitly_present;
    std::vector<std::string> m_positionals;
};

} // namespace argparse
// --- End: engine/parse_result.hpp ---


// --- Begin: engine/tokenizer.hpp ---
namespace argparse {

class tokenizer {
public:
    /**
     * @brief Classifies an individual CLI argument into a token.
     */
    [[nodiscard]] static token tokenize(std::string_view arg) noexcept {
        if (arg == "--") {
            return token{
                .type = token_type::options_delimiter,
                .raw = arg,
                .name = arg,
                .inline_value = std::nullopt
            };
        }

        if (arg.starts_with("--") && arg.size() > 2) {
            auto eq_pos = arg.find('=');
            if (eq_pos != std::string_view::npos) {
                return token{
                    .type = token_type::long_option,
                    .raw = arg,
                    .name = arg.substr(0, eq_pos),
                    .inline_value = arg.substr(eq_pos + 1)
                };
            }
            return token{
                .type = token_type::long_option,
                .raw = arg,
                .name = arg,
                .inline_value = std::nullopt
            };
        }

        if (arg.starts_with('-') && arg.size() > 1) {
            // Check if this looks like a negative number rather than an option
            // (e.g. -42, -3.14)
            if (arg.size() > 1 && (std::isdigit(static_cast<unsigned char>(arg[1])))) {
                return token{
                    .type = token_type::positional,
                    .raw = arg,
                    .name = arg,
                    .inline_value = std::nullopt
                };
            }

            auto eq_pos = arg.find('=');
            if (eq_pos != std::string_view::npos) {
                return token{
                    .type = token_type::short_option,
                    .raw = arg,
                    .name = arg.substr(0, eq_pos),
                    .inline_value = arg.substr(eq_pos + 1)
                };
            }

            return token{
                .type = token_type::short_option,
                .raw = arg,
                .name = arg,
                .inline_value = std::nullopt
            };
        }

        return token{
            .type = token_type::positional,
            .raw = arg,
            .name = arg,
            .inline_value = std::nullopt
        };
    }
};

} // namespace argparse
// --- End: engine/tokenizer.hpp ---


// --- Begin: engine/engine.hpp ---
namespace argparse {

class engine {
public:
    [[nodiscard]] static std::expected<parse_result, parse_error> parse(
        const std::vector<argument>& arguments,
        const std::vector<argument_group>& groups,
        std::span<const std::string_view> raw_args) {

        parse_result result;

        // Build lookup tables
        std::unordered_map<std::string_view, const argument*> opt_map;
        std::vector<const argument*> positional_args;

        for (const auto& arg : arguments) {
            opt_map[arg.name()] = &arg;
            if (!arg.short_name().empty()) {
                opt_map[arg.short_name()] = &arg;
                result.register_alias(arg.short_name(), arg.name());
            }
            if (arg.is_positional()) {
                positional_args.push_back(&arg);
            }
        }

        std::unordered_map<size_t, std::string> group_first_seen_arg;
        size_t positional_idx = 0;
        bool treat_all_as_positional = false;
        size_t i = 0;

        auto check_and_record_group = [&](const argument* arg, std::string_view token_raw) -> std::expected<void, parse_error> {
            if (arg->get_group_id()) {
                size_t gid = *arg->get_group_id();
                if (gid < groups.size() && groups[gid].is_mutually_exclusive()) {
                    auto it = group_first_seen_arg.find(gid);
                    if (it != group_first_seen_arg.end() && it->second != arg->name()) {
                        return std::unexpected(parse_error{
                            error_code::mutually_exclusive_conflict,
                            arg->name(),
                            token_raw,
                            std::format("Argument '{}' conflicts with previously specified '{}'", arg->name(), it->second)
                        });
                    }
                    group_first_seen_arg[gid] = arg->name();
                }
            }
            return {};
        };

        auto validate_and_store = [&](const argument* arg, std::string_view val) -> std::expected<void, parse_error> {
            if (!arg->get_choices().empty()) {
                if (std::ranges::find(arg->get_choices(), val) == arg->get_choices().end()) {
                    return std::unexpected(parse_error{
                        error_code::choice_not_allowed,
                        arg->name(),
                        val,
                        std::format("Value '{}' is not an allowed choice", val)
                    });
                }
            }

            for (const auto& v : arg->validators()) {
                auto v_res = v(val);
                if (!v_res) {
                    return std::unexpected(parse_error{
                        error_code::custom_validation_failed,
                        arg->name(),
                        val,
                        v_res.error()
                    });
                }
            }

            if (arg->get_action() == action::append) {
                result.append_value(arg->name(), std::string(val), true);
            } else {
                result.set_value(arg->name(), std::string(val), true);
            }
            return {};
        };

        while (i < raw_args.size()) {
            std::string_view current = raw_args[i];

            if (treat_all_as_positional) {
                if (positional_idx < positional_args.size()) {
                    const auto* p_arg = positional_args[positional_idx];
                    auto val_res = validate_and_store(p_arg, current);
                    if (!val_res) return std::unexpected(val_res.error());
                    if (p_arg->get_action() != action::append) {
                        positional_idx++;
                    }
                } else {
                    result.add_positional(std::string(current));
                }
                i++;
                continue;
            }

            if (current == "--") {
                treat_all_as_positional = true;
                i++;
                continue;
            }

            token tok = tokenizer::tokenize(current);

            if (tok.type == token_type::positional) {
                if (positional_idx < positional_args.size()) {
                    const auto* p_arg = positional_args[positional_idx];
                    auto val_res = validate_and_store(p_arg, current);
                    if (!val_res) return std::unexpected(val_res.error());
                    if (p_arg->get_action() != action::append) {
                        positional_idx++;
                    }
                } else {
                    result.add_positional(std::string(current));
                }
                i++;
            } else if (tok.type == token_type::long_option) {
                auto it = opt_map.find(tok.name);
                if (it == opt_map.end()) {
                    return std::unexpected(parse_error{
                        error_code::unknown_option,
                        tok.name,
                        tok.raw,
                        std::format("Unknown option '{}'", tok.name)
                    });
                }

                const argument* arg = it->second;
                auto grp_res = check_and_record_group(arg, tok.raw);
                if (!grp_res) return std::unexpected(grp_res.error());

                if (arg->is_flag()) {
                    if (tok.inline_value) {
                        auto val_res = validate_and_store(arg, *tok.inline_value);
                        if (!val_res) return std::unexpected(val_res.error());
                    } else {
                        std::string flag_val = arg->implicit_value().value_or(
                            arg->get_action() == action::store_true ? "true" : "false");
                        auto val_res = validate_and_store(arg, flag_val);
                        if (!val_res) return std::unexpected(val_res.error());
                    }
                    i++;
                } else if (arg->get_action() == action::count) {
                    size_t curr = result.count(arg->name());
                    result.set_value(arg->name(), std::format("{}", curr + 1), true);
                    i++;
                } else {
                    // Argument takes a value
                    std::string_view val;
                    if (tok.inline_value) {
                        val = *tok.inline_value;
                        i++;
                    } else {
                        if (i + 1 >= raw_args.size()) {
                            return std::unexpected(parse_error{
                                error_code::missing_value,
                                arg->name(),
                                tok.raw,
                                std::format("Option '{}' requires an argument", arg->name())
                            });
                        }
                        val = raw_args[i + 1];
                        i += 2;
                    }
                    auto val_res = validate_and_store(arg, val);
                    if (!val_res) return std::unexpected(val_res.error());
                }
            } else if (tok.type == token_type::short_option) {
                if (tok.inline_value) {
                    // e.g. -p=8080
                    auto it = opt_map.find(tok.name);
                    if (it == opt_map.end()) {
                        return std::unexpected(parse_error{
                            error_code::unknown_option,
                            tok.name,
                            tok.raw,
                            std::format("Unknown option '{}'", tok.name)
                        });
                    }
                    const argument* arg = it->second;
                    auto grp_res = check_and_record_group(arg, tok.raw);
                    if (!grp_res) return std::unexpected(grp_res.error());

                    auto val_res = validate_and_store(arg, *tok.inline_value);
                    if (!val_res) return std::unexpected(val_res.error());
                    i++;
                } else if (current.size() == 2) {
                    // Single short option, e.g. -v or -p
                    auto it = opt_map.find(current);
                    if (it == opt_map.end()) {
                        return std::unexpected(parse_error{
                            error_code::unknown_option,
                            current,
                            current,
                            std::format("Unknown option '{}'", current)
                        });
                    }

                    const argument* arg = it->second;
                    auto grp_res = check_and_record_group(arg, current);
                    if (!grp_res) return std::unexpected(grp_res.error());

                    if (arg->is_flag()) {
                        std::string flag_val = arg->implicit_value().value_or(
                            arg->get_action() == action::store_true ? "true" : "false");
                        auto val_res = validate_and_store(arg, flag_val);
                        if (!val_res) return std::unexpected(val_res.error());
                        i++;
                    } else if (arg->get_action() == action::count) {
                        size_t curr = result.count(arg->name());
                        result.set_value(arg->name(), std::format("{}", curr + 1), true);
                        i++;
                    } else {
                        // Takes a value
                        if (i + 1 >= raw_args.size()) {
                            return std::unexpected(parse_error{
                                error_code::missing_value,
                                arg->name(),
                                current,
                                std::format("Option '{}' requires an argument", arg->name())
                            });
                        }
                        std::string_view val = raw_args[i + 1];
                        auto val_res = validate_and_store(arg, val);
                        if (!val_res) return std::unexpected(val_res.error());
                        i += 2;
                    }
                } else {
                    // Short option chaining (e.g. -xvf) or inline value (e.g. -p8080)
                    std::string first_opt = std::format("-{}", current[1]);
                    auto it = opt_map.find(first_opt);
                    if (it == opt_map.end()) {
                        return std::unexpected(parse_error{
                            error_code::unknown_option,
                            first_opt,
                            current,
                            std::format("Unknown option '{}'", first_opt)
                        });
                    }

                    const argument* first_arg = it->second;
                    if (first_arg->takes_value()) {
                        // Inline value: -p8080 -> -p with value 8080
                        auto grp_res = check_and_record_group(first_arg, current);
                        if (!grp_res) return std::unexpected(grp_res.error());

                        std::string_view val = current.substr(2);
                        auto val_res = validate_and_store(first_arg, val);
                        if (!val_res) return std::unexpected(val_res.error());
                        i++;
                    } else {
                        // Flag chaining: -xvf
                        for (size_t c = 1; c < current.size(); ++c) {
                            std::string opt_char = std::format("-{}", current[c]);
                            auto opt_it = opt_map.find(opt_char);
                            if (opt_it == opt_map.end()) {
                                return std::unexpected(parse_error{
                                    error_code::unknown_option,
                                    opt_char,
                                    current,
                                    std::format("Unknown option '{}' in flag chain", opt_char)
                                });
                            }

                            const argument* chained_arg = opt_it->second;
                            auto grp_res = check_and_record_group(chained_arg, current);
                            if (!grp_res) return std::unexpected(grp_res.error());

                            if (chained_arg->is_flag()) {
                                std::string flag_val = chained_arg->implicit_value().value_or(
                                    chained_arg->get_action() == action::store_true ? "true" : "false");
                                auto val_res = validate_and_store(chained_arg, flag_val);
                                if (!val_res) return std::unexpected(val_res.error());
                            } else if (chained_arg->get_action() == action::count) {
                                size_t curr = result.count(chained_arg->name());
                                result.set_value(chained_arg->name(), std::format("{}", curr + 1), true);
                            } else if (chained_arg->takes_value()) {
                                // Remaining chars in the chain form the value!
                                std::string_view val;
                                if (c + 1 < current.size()) {
                                    val = current.substr(c + 1);
                                } else {
                                    if (i + 1 >= raw_args.size()) {
                                        return std::unexpected(parse_error{
                                            error_code::missing_value,
                                            chained_arg->name(),
                                            current,
                                            std::format("Option '{}' requires an argument", chained_arg->name())
                                        });
                                    }
                                    val = raw_args[i + 1];
                                    i++; // consume next
                                }
                                auto val_res = validate_and_store(chained_arg, val);
                                if (!val_res) return std::unexpected(val_res.error());
                                break; // end chaining loop
                            }
                        }
                        i++;
                    }
                }
            }
        }

        // Post-parsing: apply defaults
        for (const auto& arg : arguments) {
            if (!result.has(arg.name())) {
                if (arg.default_value()) {
                    result.set_value(arg.name(), *arg.default_value(), false);
                }
            }
        }

        // If --help was requested, bypass required constraint checks
        bool help_requested = result.has("--help") || result.has("-h");
        if (help_requested) {
            return result;
        }

        for (const auto& arg : arguments) {
            if (!result.has(arg.name()) && arg.is_required()) {
                return std::unexpected(parse_error{
                    error_code::missing_required_argument,
                    arg.name(),
                    "",
                    std::format("Required argument '{}' was not provided", arg.name())
                });
            }
        }

        // Check required mutually exclusive groups
        for (const auto& group : groups) {
            if (group.is_mutually_exclusive() && group.is_required()) {
                if (!group_first_seen_arg.contains(group.id())) {
                    return std::unexpected(parse_error{
                        error_code::missing_required_argument,
                        "",
                        "",
                        "One of the mutually exclusive arguments must be provided"
                    });
                }
            }
        }

        return result;
    }
};

} // namespace argparse
// --- End: engine/engine.hpp ---


// --- Begin: format/formatter.hpp ---
namespace argparse {

class formatter {
public:
    [[nodiscard]] static std::string format_help(
        std::string_view program_name,
        std::string_view description,
        std::string_view epilog,
        const std::vector<argument>& arguments,
        [[maybe_unused]] const std::vector<argument_group>& groups) {

        std::ostringstream oss;

        // 1. Usage line
        oss << "Usage: " << program_name;

        bool has_options = false;
        std::vector<std::string> pos_usage;

        for (const auto& arg : arguments) {
            if (arg.is_positional()) {
                if (arg.is_required()) {
                    pos_usage.push_back(std::format("<{}>", arg.metavar()));
                } else {
                    pos_usage.push_back(std::format("[<{}>]", arg.metavar()));
                }
            } else {
                has_options = true;
            }
        }

        if (has_options) {
            oss << " [options]";
        }

        for (const auto& p : pos_usage) {
            oss << " " << p;
        }
        oss << "\n\n";

        // 2. Description
        if (!description.empty()) {
            oss << description << "\n\n";
        }

        // 3. Positional Arguments
        std::vector<std::pair<std::string, std::string>> pos_entries;
        for (const auto& arg : arguments) {
            if (arg.is_positional()) {
                std::string label = arg.metavar();
                std::string desc = arg.help();
                if (arg.default_value()) {
                    desc += std::format(" (default: {})", *arg.default_value());
                }
                if (!arg.get_choices().empty()) {
                    desc += " [choices: ";
                    for (size_t i = 0; i < arg.get_choices().size(); ++i) {
                        desc += arg.get_choices()[i];
                        if (i + 1 < arg.get_choices().size()) desc += ", ";
                    }
                    desc += "]";
                }
                pos_entries.emplace_back(std::move(label), std::move(desc));
            }
        }

        if (!pos_entries.empty()) {
            oss << "Positional arguments:\n";
            size_t max_label_len = 0;
            for (const auto& [label, _] : pos_entries) {
                max_label_len = std::max(max_label_len, label.size());
            }

            for (const auto& [label, desc] : pos_entries) {
                oss << "  " << label;
                if (label.size() < max_label_len) {
                    oss << std::string(max_label_len - label.size(), ' ');
                }
                oss << "    " << desc << "\n";
            }
            oss << "\n";
        }

        // 4. Options
        std::vector<std::pair<std::string, std::string>> opt_entries;
        for (const auto& arg : arguments) {
            if (!arg.is_positional()) {
                std::string label;
                if (!arg.short_name().empty()) {
                    label = arg.short_name();
                    if (arg.takes_value()) {
                        label += std::format(" <{}>", arg.metavar());
                    }
                    label += ", ";
                }
                label += arg.name();
                if (arg.takes_value()) {
                    label += std::format(" <{}>", arg.metavar());
                }

                std::string desc = arg.help();
                if (arg.is_required()) {
                    desc += " (required)";
                }
                if (arg.default_value()) {
                    desc += std::format(" (default: {})", *arg.default_value());
                }
                if (!arg.get_choices().empty()) {
                    desc += " [choices: ";
                    for (size_t i = 0; i < arg.get_choices().size(); ++i) {
                        desc += arg.get_choices()[i];
                        if (i + 1 < arg.get_choices().size()) desc += ", ";
                    }
                    desc += "]";
                }

                opt_entries.emplace_back(std::move(label), std::move(desc));
            }
        }

        if (!opt_entries.empty()) {
            oss << "Options:\n";
            size_t max_label_len = 0;
            for (const auto& [label, _] : opt_entries) {
                max_label_len = std::max(max_label_len, label.size());
            }

            for (const auto& [label, desc] : opt_entries) {
                oss << "  " << label;
                if (label.size() < max_label_len) {
                    oss << std::string(max_label_len - label.size(), ' ');
                }
                oss << "    " << desc << "\n";
            }
            oss << "\n";
        }

        // 5. Epilog
        if (!epilog.empty()) {
            oss << epilog << "\n";
        }

        return oss.str();
    }

    [[nodiscard]] static std::string format_version(
        std::string_view program_name,
        std::string_view version) {
        return std::format("{} {}\n", program_name, version);
    }
};

} // namespace argparse
// --- End: format/formatter.hpp ---


// --- Begin: config/parser.hpp ---
namespace argparse {

class argument_parser {
public:
    explicit argument_parser(std::string_view program_name, std::string_view description = "")
        : m_program_name(program_name), m_description(description) {
        add_argument("--help", "-h").help("Show this help message and exit").flag();
    }

    argument_parser& description(std::string_view desc) {
        m_description = std::string(desc);
        return *this;
    }

    argument_parser& epilog(std::string_view epi) {
        m_epilog = std::string(epi);
        return *this;
    }

    argument_parser& version(std::string_view ver) {
        m_version = std::string(ver);
        return *this;
    }

    argument& add_argument(std::string_view name, std::string_view short_name = "") {
        m_arguments.emplace_back(name, short_name);
        return m_arguments.back();
    }

    argument_group& add_mutually_exclusive_group(bool required = false) {
        size_t id = m_groups.size();
        m_groups.emplace_back(id, *this, true, required);
        return m_groups.back();
    }

    argument_group& add_group(bool required = false) {
        size_t id = m_groups.size();
        m_groups.emplace_back(id, *this, false, required);
        return m_groups.back();
    }

    [[nodiscard]] std::string format_help() const {
        return formatter::format_help(m_program_name, m_description, m_epilog, m_arguments, m_groups);
    }

    [[nodiscard]] std::string format_version() const {
        return formatter::format_version(m_program_name, m_version);
    }

    /**
     * @brief Pure functional parse entry point for string_view spans.
     * Guaranteed noexcept and zero string copies on parse path.
     */
    [[nodiscard]] std::expected<parse_result, parse_error>
    parse_args(std::span<const std::string_view> args) const noexcept {
        return engine::parse(m_arguments, m_groups, args);
    }

    /**
     * @brief Convenient parse entry point for standard argc/argv.
     */
    [[nodiscard]] std::expected<parse_result, parse_error>
    parse_args(int argc, const char* const* argv) const {
        if (argc <= 1) {
            return parse_args(std::span<const std::string_view>{});
        }
        std::vector<std::string_view> views;
        views.reserve(static_cast<size_t>(argc - 1));
        for (int i = 1; i < argc; ++i) {
            views.emplace_back(argv[i]);
        }
        return parse_args(views);
    }

    /**
     * @brief Exception-throwing parse variant for script-style applications.
     */
    [[nodiscard]] parse_result parse_or_throw(std::span<const std::string_view> args) const {
        auto res = parse_args(args);
        if (!res) {
            throw std::runtime_error(res.error().to_string());
        }
        return std::move(*res);
    }

    [[nodiscard]] parse_result parse_or_throw(int argc, const char* const* argv) const {
        auto res = parse_args(argc, argv);
        if (!res) {
            throw std::runtime_error(res.error().to_string());
        }
        return std::move(*res);
    }

    /**
     * @brief Parse arguments or print error and exit with code 1.
     */
    [[nodiscard]] parse_result parse_or_exit(int argc, const char* const* argv) const {
        auto res = parse_args(argc, argv);
        if (!res) {
            std::cerr << res.error().to_string() << "\n\n";
            std::cerr << format_help() << "\n";
            std::exit(1);
        }
        if (res->has("--help") || res->has("-h")) {
            std::cout << format_help();
            std::exit(0);
        }
        if (res->has("--version")) {
            std::cout << format_version();
            std::exit(0);
        }
        return std::move(*res);
    }

    // Accessors
    [[nodiscard]] const std::string& program_name() const noexcept { return m_program_name; }
    [[nodiscard]] const std::string& get_description() const noexcept { return m_description; }
    [[nodiscard]] const std::string& get_version() const noexcept { return m_version; }
    [[nodiscard]] const std::vector<argument>& arguments() const noexcept { return m_arguments; }
    [[nodiscard]] const std::vector<argument_group>& groups() const noexcept { return m_groups; }

private:
    std::string m_program_name;
    std::string m_description;
    std::string m_epilog;
    std::string m_version{"1.0.0"};
    std::vector<argument> m_arguments;
    std::vector<argument_group> m_groups;
};

inline argument& argument_group::add_argument(std::string_view name, std::string_view short_name) {
    auto& arg = m_parent->add_argument(name, short_name);
    arg.group_id(m_id);
    register_argument_name(name);
    return arg;
}

} // namespace argparse
// --- End: config/parser.hpp ---

