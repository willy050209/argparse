#pragma once

#include <argparse/config/argument.hpp>
#include <argparse/config/argument_group.hpp>
#include <argparse/core/error.hpp>
#include <argparse/engine/engine.hpp>
#include <argparse/engine/parse_result.hpp>
#include <argparse/format/formatter.hpp>

#include <cstdlib>
#include <expected>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

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
