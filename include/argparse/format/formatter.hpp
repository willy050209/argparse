#pragma once

#include <argparse/config/argument.hpp>
#include <argparse/config/argument_group.hpp>

#include <algorithm>
#include <format>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace argparse {

class formatter {
public:
    [[nodiscard]] static std::string format_help(
        std::string_view program_name,
        std::string_view description,
        std::string_view epilog,
        const std::vector<argument>& arguments,
        [[maybe_unused]] const std::vector<argument_group>& groups) {

        std::ostringstream oss;

        // 1. Usage line
        oss << "Usage: " << program_name;

        bool has_options = false;
        std::vector<std::string> pos_usage;

        for (const auto& arg : arguments) {
            if (arg.is_positional()) {
                if (arg.is_required()) {
                    pos_usage.push_back(std::format("<{}>", arg.metavar()));
                } else {
                    pos_usage.push_back(std::format("[<{}>]", arg.metavar()));
                }
            } else {
                has_options = true;
            }
        }

        if (has_options) {
            oss << " [options]";
        }

        for (const auto& p : pos_usage) {
            oss << " " << p;
        }
        oss << "\n\n";

        // 2. Description
        if (!description.empty()) {
            oss << description << "\n\n";
        }

        // 3. Positional Arguments
        std::vector<std::pair<std::string, std::string>> pos_entries;
        for (const auto& arg : arguments) {
            if (arg.is_positional()) {
                std::string label = arg.metavar();
                std::string desc = arg.help();
                if (arg.default_value()) {
                    desc += std::format(" (default: {})", *arg.default_value());
                }
                if (!arg.get_choices().empty()) {
                    desc += " [choices: ";
                    for (size_t i = 0; i < arg.get_choices().size(); ++i) {
                        desc += arg.get_choices()[i];
                        if (i + 1 < arg.get_choices().size()) desc += ", ";
                    }
                    desc += "]";
                }
                pos_entries.emplace_back(std::move(label), std::move(desc));
            }
        }

        if (!pos_entries.empty()) {
            oss << "Positional arguments:\n";
            size_t max_label_len = 0;
            for (const auto& [label, _] : pos_entries) {
                max_label_len = std::max(max_label_len, label.size());
            }

            for (const auto& [label, desc] : pos_entries) {
                oss << "  " << label;
                if (label.size() < max_label_len) {
                    oss << std::string(max_label_len - label.size(), ' ');
                }
                oss << "    " << desc << "\n";
            }
            oss << "\n";
        }

        // 4. Options
        std::vector<std::pair<std::string, std::string>> opt_entries;
        for (const auto& arg : arguments) {
            if (!arg.is_positional()) {
                std::string label;
                if (!arg.short_name().empty()) {
                    label = arg.short_name();
                    if (arg.takes_value()) {
                        label += std::format(" <{}>", arg.metavar());
                    }
                    label += ", ";
                }
                label += arg.name();
                if (arg.takes_value()) {
                    label += std::format(" <{}>", arg.metavar());
                }

                std::string desc = arg.help();
                if (arg.is_required()) {
                    desc += " (required)";
                }
                if (arg.default_value()) {
                    desc += std::format(" (default: {})", *arg.default_value());
                }
                if (!arg.get_choices().empty()) {
                    desc += " [choices: ";
                    for (size_t i = 0; i < arg.get_choices().size(); ++i) {
                        desc += arg.get_choices()[i];
                        if (i + 1 < arg.get_choices().size()) desc += ", ";
                    }
                    desc += "]";
                }

                opt_entries.emplace_back(std::move(label), std::move(desc));
            }
        }

        if (!opt_entries.empty()) {
            oss << "Options:\n";
            size_t max_label_len = 0;
            for (const auto& [label, _] : opt_entries) {
                max_label_len = std::max(max_label_len, label.size());
            }

            for (const auto& [label, desc] : opt_entries) {
                oss << "  " << label;
                if (label.size() < max_label_len) {
                    oss << std::string(max_label_len - label.size(), ' ');
                }
                oss << "    " << desc << "\n";
            }
            oss << "\n";
        }

        // 5. Epilog
        if (!epilog.empty()) {
            oss << epilog << "\n";
        }

        return oss.str();
    }

    [[nodiscard]] static std::string format_version(
        std::string_view program_name,
        std::string_view version) {
        return std::format("{} {}\n", program_name, version);
    }
};

} // namespace argparse
