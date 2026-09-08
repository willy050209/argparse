module;

#include <argparse/argparse.hpp>

export module argparse;

export namespace argparse {
    using argparse::argument_parser;
    using argparse::argument;
    using argparse::argument_group;
    using argparse::parse_result;
    using argparse::parse_error;
    using argparse::error_code;
    using argparse::action;
    using argparse::token;
    using argparse::token_type;
    using argparse::value_parser;
    using argparse::cli_model;
    using argparse::argument_model;
    using argparse::formatter;

    namespace compat {
        using argparse::compat::print;
        using argparse::compat::println;
    }
    using compat::print;
    using compat::println;
}
