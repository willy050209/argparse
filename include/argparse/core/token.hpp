#pragma once

#include <argparse/compat/string_view.hpp>

namespace argparse {

enum class token_type {
    long_option,      // e.g. --verbose or --port=8080
    short_option,     // e.g. -v or -p8080 or chained -xvf
    positional,       // e.g. filename.txt
    options_delimiter // explicitly --
};

struct token {
    token_type type{token_type::positional};
    string_view raw{};
    string_view name{};
    bool has_inline_value{false};
    string_view inline_value{};

    constexpr token() noexcept = default;
    constexpr token(token_type t, string_view r, string_view n, bool has_val, string_view val) noexcept
        : type(t), raw(r), name(n), has_inline_value(has_val), inline_value(val) {}
};

} // namespace argparse
