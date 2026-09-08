#pragma once

#include <argparse/config/action.hpp>
#include <argparse/config/argument.hpp>
#include <argparse/config/argument_group.hpp>

#include <sstream>
#include <string>
#include <vector>

namespace argparse {

struct argument_model {
    std::string name;
    std::string short_name;
    std::string help;
    std::string metavar;
    action act{action::store};
    bool is_required{false};
    bool is_flag{false};
    bool is_positional{false};
    std::string default_value;
    std::string implicit_value;
    std::vector<std::string> choices;
};

struct cli_model {
    std::string program_name;
    std::string description;
    std::string epilog;
    std::string version{"1.0.0"};
    std::vector<argument_model> arguments;
    std::vector<std::vector<std::string>> mutually_exclusive_groups;

    [[nodiscard]] std::string to_json() const {
        std::ostringstream oss;
        auto escape_json = [](const std::string& s) -> std::string {
            std::string out;
            for (char c : s) {
                if (c == '"') out += "\\\"";
                else if (c == '\\') out += "\\\\";
                else if (c == '\n') out += "\\n";
                else if (c == '\r') out += "\\r";
                else if (c == '\t') out += "\\t";
                else out += c;
            }
            return out;
        };

        oss << "{\n";
        oss << "  \"program_name\": \"" << escape_json(program_name) << "\",\n";
        oss << "  \"description\": \"" << escape_json(description) << "\",\n";
        oss << "  \"version\": \"" << escape_json(version) << "\",\n";
        oss << "  \"arguments\": [\n";

        for (size_t i = 0; i < arguments.size(); ++i) {
            const auto& a = arguments[i];
            oss << "    {\n";
            oss << "      \"name\": \"" << escape_json(a.name) << "\",\n";
            oss << "      \"short_name\": \"" << escape_json(a.short_name) << "\",\n";
            oss << "      \"help\": \"" << escape_json(a.help) << "\",\n";
            oss << "      \"metavar\": \"" << escape_json(a.metavar) << "\",\n";
            oss << "      \"is_required\": " << (a.is_required ? "true" : "false") << ",\n";
            oss << "      \"is_flag\": " << (a.is_flag ? "true" : "false") << ",\n";
            oss << "      \"is_positional\": " << (a.is_positional ? "true" : "false") << ",\n";
            oss << "      \"default_value\": \"" << escape_json(a.default_value) << "\",\n";
            oss << "      \"implicit_value\": \"" << escape_json(a.implicit_value) << "\",\n";
            oss << "      \"choices\": [";
            for (size_t c = 0; c < a.choices.size(); ++c) {
                oss << "\"" << escape_json(a.choices[c]) << "\"";
                if (c + 1 < a.choices.size()) oss << ", ";
            }
            oss << "]\n";
            oss << "    }" << (i + 1 < arguments.size() ? "," : "") << "\n";
        }

        oss << "  ],\n";
        oss << "  \"mutually_exclusive_groups\": [\n";
        for (size_t g = 0; g < mutually_exclusive_groups.size(); ++g) {
            oss << "    [";
            for (size_t m = 0; m < mutually_exclusive_groups[g].size(); ++m) {
                oss << "\"" << escape_json(mutually_exclusive_groups[g][m]) << "\"";
                if (m + 1 < mutually_exclusive_groups[g].size()) oss << ", ";
            }
            oss << "]" << (g + 1 < mutually_exclusive_groups.size() ? "," : "") << "\n";
        }
        oss << "  ]\n";
        oss << "}\n";
        return oss.str();
    }
};

} // namespace argparse
