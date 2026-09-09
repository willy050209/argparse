#pragma once

#include <algorithm>
#include <argparse/compat/string_view.hpp>
#include <argparse/config/argument.hpp>
#include <argparse/config/argument_group.hpp>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace argparse {

class formatter {
public:
    [[nodiscard]] static std::string format_help(string_view program_name, string_view description, string_view epilog,
                                                 const std::vector<argument> &arguments,
                                                 [[maybe_unused]] const std::vector<argument_group> &groups) {

        std::ostringstream oss;

        oss << "Usage: " << program_name;

        bool has_options = false;
        std::vector<std::string> pos_usage;

        for (const auto &arg : arguments) {
            if (arg.is_positional()) {
                if (arg.is_required()) {
                    pos_usage.push_back("<" + arg.metavar() + ">");
                } else {
                    pos_usage.push_back("[<" + arg.metavar() + ">]");
                }
            } else {
                has_options = true;
            }
        }

        if (has_options) {
            oss << " [options]";
        }

        for (const auto &p : pos_usage) {
            oss << " " << p;
        }
        oss << "\n\n";

        if (!description.empty()) {
            oss << description << "\n\n";
        }

        std::vector<std::pair<std::string, std::string>> pos_entries;
        for (const auto &arg : arguments) {
            if (arg.is_positional()) {
                std::string label = arg.metavar();
                std::string desc = arg.help();
                if (arg.has_default()) {
                    desc += " (default: " + arg.default_value() + ")";
                }
                if (!arg.get_choices().empty()) {
                    desc += " [choices: ";
                    for (size_t i = 0; i < arg.get_choices().size(); ++i) {
                        desc += arg.get_choices()[i];
                        if (i + 1 < arg.get_choices().size())
                            desc += ", ";
                    }
                    desc += "]";
                }
                pos_entries.emplace_back(std::move(label), std::move(desc));
            }
        }

        if (!pos_entries.empty()) {
            oss << "Positional arguments:\n";
            size_t max_label_len = 0;
            for (const auto &entry : pos_entries) {
                max_label_len = std::max(max_label_len, entry.first.size());
            }

            for (const auto &entry : pos_entries) {
                oss << "  " << entry.first;
                if (entry.first.size() < max_label_len) {
                    oss << std::string(max_label_len - entry.first.size(), ' ');
                }
                oss << "    " << entry.second << "\n";
            }
            oss << "\n";
        }

        std::vector<std::pair<std::string, std::string>> opt_entries;
        for (const auto &arg : arguments) {
            if (!arg.is_positional()) {
                std::string label;
                if (!arg.short_name().empty()) {
                    label = arg.short_name();
                    if (arg.takes_value()) {
                        label += " <" + arg.metavar() + ">";
                    }
                    label += ", ";
                }
                label += arg.name();
                if (arg.takes_value()) {
                    label += " <" + arg.metavar() + ">";
                }

                std::string desc = arg.help();
                if (arg.is_required()) {
                    desc += " (required)";
                }
                if (arg.has_default()) {
                    desc += " (default: " + arg.default_value() + ")";
                }
                if (!arg.get_choices().empty()) {
                    desc += " [choices: ";
                    for (size_t i = 0; i < arg.get_choices().size(); ++i) {
                        desc += arg.get_choices()[i];
                        if (i + 1 < arg.get_choices().size())
                            desc += ", ";
                    }
                    desc += "]";
                }

                opt_entries.emplace_back(std::move(label), std::move(desc));
            }
        }

        if (!opt_entries.empty()) {
            oss << "Options:\n";
            size_t max_label_len = 0;
            for (const auto &entry : opt_entries) {
                max_label_len = std::max(max_label_len, entry.first.size());
            }

            for (const auto &entry : opt_entries) {
                oss << "  " << entry.first;
                if (entry.first.size() < max_label_len) {
                    oss << std::string(max_label_len - entry.first.size(), ' ');
                }
                oss << "    " << entry.second << "\n";
            }
            oss << "\n";
        }

        if (!epilog.empty()) {
            oss << epilog << "\n";
        }

        return oss.str();
    }

    [[nodiscard]] static std::string format_version(string_view program_name, string_view version) {
        std::ostringstream oss;
        oss << program_name << " " << version << "\n";
        return oss.str();
    }
};

} // namespace argparse
