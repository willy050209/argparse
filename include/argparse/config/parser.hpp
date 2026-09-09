#pragma once

#include <argparse/compat/detect.hpp>
#include <argparse/compat/expected.hpp>
#include <argparse/compat/print.hpp>
#include <argparse/compat/span.hpp>
#include <argparse/compat/string_view.hpp>
#include <argparse/config/argument.hpp>
#include <argparse/config/argument_group.hpp>
#include <argparse/core/error.hpp>
#include <argparse/engine/engine.hpp>
#include <argparse/engine/parse_result.hpp>
#include <argparse/format/formatter.hpp>
#include <argparse/model/cli_model.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace argparse {

class argument_parser {
public:
    explicit argument_parser(string_view program_name, string_view description = "")
        : m_program_name(program_name.data(), program_name.size()),
          m_description(description.data(), description.size()) {
        add_argument("--help", "-h").help("Show this help message and exit").flag();
    }

    argument_parser &description(string_view desc) {
        m_description = std::string(desc.data(), desc.size());
        return *this;
    }

    argument_parser &epilog(string_view epi) {
        m_epilog = std::string(epi.data(), epi.size());
        return *this;
    }

    argument_parser &version(string_view ver) {
        m_version = std::string(ver.data(), ver.size());
        return *this;
    }

    argument &add_argument(string_view name, string_view short_name = "") {
        m_arguments.emplace_back(name, short_name);
        return m_arguments.back();
    }

    argument_group &add_mutually_exclusive_group(bool required = false) {
        size_t id = m_groups.size();
        m_groups.emplace_back(id, *this, true, required);
        return m_groups.back();
    }

    argument_group &add_group(bool required = false) {
        size_t id = m_groups.size();
        m_groups.emplace_back(id, *this, false, required);
        return m_groups.back();
    }

    [[nodiscard]] std::string format_help() const {
        return formatter::format_help(string_view(m_program_name), string_view(m_description), string_view(m_epilog),
                                      m_arguments, m_groups);
    }

    [[nodiscard]] std::string format_version() const {
        return formatter::format_version(string_view(m_program_name), string_view(m_version));
    }

    [[nodiscard]] cli_model export_model() const {
        cli_model model;
        model.program_name = m_program_name;
        model.description = m_description;
        model.epilog = m_epilog;
        model.version = m_version;

        for (const auto &arg : m_arguments) {
            argument_model am;
            am.name = arg.name();
            am.short_name = arg.short_name();
            am.help = arg.help();
            am.metavar = arg.metavar();
            am.act = arg.get_action();
            am.is_required = arg.is_required();
            am.is_flag = arg.is_flag();
            am.is_positional = arg.is_positional();
            am.default_value = arg.has_default() ? arg.default_value() : "";
            am.implicit_value = arg.has_implicit() ? arg.implicit_value() : "";
            am.choices = arg.get_choices();
            model.arguments.push_back(std::move(am));
        }

        for (const auto &grp : m_groups) {
            if (grp.is_mutually_exclusive()) {
                model.mutually_exclusive_groups.push_back(grp.argument_names());
            }
        }
        return model;
    }

    [[nodiscard]] std::string to_json() const { return export_model().to_json(); }

    [[nodiscard]] expected<parse_result, parse_error> parse_args(span<const string_view> args) const noexcept {
        return engine::parse(m_arguments, m_groups, args);
    }

    [[nodiscard]] expected<parse_result, parse_error> parse_args(int argc, const char *const *argv) const {
        if (argc <= 1) {
            return parse_args(span<const string_view>{});
        }
        std::vector<string_view> views;
        views.reserve(static_cast<size_t>(argc - 1));
        for (int i = 1; i < argc; ++i) {
            views.emplace_back(argv[i]);
        }
        return parse_args(span<const string_view>(views));
    }

    [[nodiscard]] parse_result parse_or_throw(span<const string_view> args) const {
        auto res = parse_args(args);
        if (!res) {
            throw std::runtime_error(res.error().to_string());
        }
        return *res;
    }

    [[nodiscard]] parse_result parse_or_throw(int argc, const char *const *argv) const {
        auto res = parse_args(argc, argv);
        if (!res) {
            throw std::runtime_error(res.error().to_string());
        }
        return *res;
    }

    [[nodiscard]] parse_result parse_or_exit(int argc, const char *const *argv) const {
        auto res = parse_args(argc, argv);
        if (!res) {
            compat::println(std::cerr, "{}", res.error().to_string());
            compat::println(std::cerr);
            compat::print(std::cerr, "{}", format_help());
            std::exit(1);
        }
        if (res->has("--help") || res->has("-h")) {
            compat::print("{}", format_help());
            std::exit(0);
        }
        if (res->has("--version")) {
            compat::print("{}", format_version());
            std::exit(0);
        }
        return *res;
    }

    // Accessors
    [[nodiscard]] const std::string &program_name() const noexcept { return m_program_name; }
    [[nodiscard]] const std::string &get_description() const noexcept { return m_description; }
    [[nodiscard]] const std::string &get_version() const noexcept { return m_version; }
    [[nodiscard]] const std::vector<argument> &arguments() const noexcept { return m_arguments; }
    [[nodiscard]] const std::vector<argument_group> &groups() const noexcept { return m_groups; }

private:
    std::string m_program_name;
    std::string m_description;
    std::string m_epilog;
    std::string m_version{"1.0.0"};
    std::vector<argument> m_arguments;
    std::vector<argument_group> m_groups;
};

inline argument &argument_group::add_argument(string_view name, string_view short_name) {
    auto &arg = m_parent->add_argument(name, short_name);
    arg.group_id(m_id);
    register_argument_name(name);
    return arg;
}

} // namespace argparse
