#pragma once

#include <optional>
#include <string_view>

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
