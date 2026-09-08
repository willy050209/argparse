#pragma once

#include <argparse/core/token.hpp>

#include <optional>
#include <string_view>
#include <vector>

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
