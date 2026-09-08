#pragma once

#include <argparse/core/error.hpp>

#include <concepts>
#include <expected>
#include <string_view>
#include <type_traits>
#include <vector>

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
