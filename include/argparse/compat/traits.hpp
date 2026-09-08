#pragma once

#include <argparse/compat/detect.hpp>
#include <argparse/compat/expected.hpp>
#include <argparse/compat/string_view.hpp>

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#if ARGPARSE_HAS_CONCEPTS
    #include <concepts>
#endif

namespace argparse {

template <bool B, typename T = void>
using enable_if_t = typename std::enable_if<B, T>::type;

template <typename...>
using void_t = void;

struct parse_error;

template <typename T, typename = void>
struct value_parser;

#if ARGPARSE_HAS_CONCEPTS
    template <typename T>
    concept parsable = requires(string_view sv) {
        { value_parser<T>::parse(sv) } -> std::same_as<expected<T, parse_error>>;
    };

    template <typename F, typename T>
    concept boolean_validator_for = requires(F&& f, const T& val) {
        { std::forward<F>(f)(val) } -> std::convertible_to<bool>;
    };

    template <typename F, typename T>
    concept result_validator_for = requires(F&& f, const T& val) {
        { std::forward<F>(f)(val) } -> std::same_as<expected<void, std::string>>;
    };

    template <typename F, typename T>
    concept validator_for = boolean_validator_for<F, T> || result_validator_for<F, T>;

    #define ARGPARSE_REQUIRES_PARSABLE(T) requires parsable<T>
    #define ARGPARSE_REQUIRES_PARSABLE_AND_VALIDATOR(T, F) requires parsable<T> && validator_for<F, T>
#else
    template <typename T, typename = void>
    struct is_parsable : std::false_type {};

    template <typename T>
    struct is_parsable<T, void_t<decltype(value_parser<T>::parse(std::declval<string_view>()))>> : std::true_type {};

#if ARGPARSE_CPLUSPLUS >= 201402L
    template <typename T>
    inline constexpr bool is_parsable_v = is_parsable<T>::value;
#endif

    #define ARGPARSE_REQUIRES_PARSABLE(T)
    #define ARGPARSE_REQUIRES_PARSABLE_AND_VALIDATOR(T, F)
#endif

template <typename T>
struct is_vector : std::false_type {};

template <typename T, typename Alloc>
struct is_vector<std::vector<T, Alloc>> : std::true_type {};

#if ARGPARSE_CPLUSPLUS >= 201402L
template <typename T>
inline constexpr bool is_vector_v = is_vector<T>::value;
#endif

} // namespace argparse
