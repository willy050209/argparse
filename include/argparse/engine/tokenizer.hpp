#pragma once

#include <argparse/compat/string_view.hpp>
#include <argparse/core/token.hpp>

#include <cctype>

namespace argparse {

class tokenizer {
public:
    [[nodiscard]] static token tokenize(string_view arg) noexcept {
        if (arg == "--") {
            return token{
                token_type::options_delimiter,
                arg,
                arg,
                false,
                string_view{}
            };
        }

        if (starts_with(arg, "--") && arg.size() > 2) {
            auto eq_pos = arg.find('=');
            if (eq_pos != string_view::npos) {
                return token{
                    token_type::long_option,
                    arg,
                    arg.substr(0, eq_pos),
                    true,
                    arg.substr(eq_pos + 1)
                };
            }
            return token{
                token_type::long_option,
                arg,
                arg,
                false,
                string_view{}
            };
        }

        if (starts_with(arg, '-') && arg.size() > 1) {
            if (arg.size() > 1 && (std::isdigit(static_cast<unsigned char>(arg[1])))) {
                return token{
                    token_type::positional,
                    arg,
                    arg,
                    false,
                    string_view{}
                };
            }

            auto eq_pos = arg.find('=');
            if (eq_pos != string_view::npos) {
                return token{
                    token_type::short_option,
                    arg,
                    arg.substr(0, eq_pos),
                    true,
                    arg.substr(eq_pos + 1)
                };
            }

            return token{
                token_type::short_option,
                arg,
                arg,
                false,
                string_view{}
            };
        }

        return token{
            token_type::positional,
            arg,
            arg,
            false,
            string_view{}
        };
    }
};

} // namespace argparse
