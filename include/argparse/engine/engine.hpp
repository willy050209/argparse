#pragma once

#include <argparse/config/argument.hpp>
#include <argparse/config/argument_group.hpp>
#include <argparse/core/error.hpp>
#include <argparse/engine/parse_result.hpp>
#include <argparse/engine/tokenizer.hpp>

#include <algorithm>
#include <expected>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace argparse {

class engine {
public:
    [[nodiscard]] static std::expected<parse_result, parse_error> parse(
        const std::vector<argument>& arguments,
        const std::vector<argument_group>& groups,
        std::span<const std::string_view> raw_args) {

        parse_result result;

        // Build lookup tables
        std::unordered_map<std::string_view, const argument*> opt_map;
        std::vector<const argument*> positional_args;

        for (const auto& arg : arguments) {
            opt_map[arg.name()] = &arg;
            if (!arg.short_name().empty()) {
                opt_map[arg.short_name()] = &arg;
                result.register_alias(arg.short_name(), arg.name());
            }
            if (arg.is_positional()) {
                positional_args.push_back(&arg);
            }
        }

        std::unordered_map<size_t, std::string> group_first_seen_arg;
        size_t positional_idx = 0;
        bool treat_all_as_positional = false;
        size_t i = 0;

        auto check_and_record_group = [&](const argument* arg, std::string_view token_raw) -> std::expected<void, parse_error> {
            if (arg->get_group_id()) {
                size_t gid = *arg->get_group_id();
                if (gid < groups.size() && groups[gid].is_mutually_exclusive()) {
                    auto it = group_first_seen_arg.find(gid);
                    if (it != group_first_seen_arg.end() && it->second != arg->name()) {
                        return std::unexpected(parse_error{
                            error_code::mutually_exclusive_conflict,
                            arg->name(),
                            token_raw,
                            std::format("Argument '{}' conflicts with previously specified '{}'", arg->name(), it->second)
                        });
                    }
                    group_first_seen_arg[gid] = arg->name();
                }
            }
            return {};
        };

        auto validate_and_store = [&](const argument* arg, std::string_view val) -> std::expected<void, parse_error> {
            if (!arg->get_choices().empty()) {
                if (std::ranges::find(arg->get_choices(), val) == arg->get_choices().end()) {
                    return std::unexpected(parse_error{
                        error_code::choice_not_allowed,
                        arg->name(),
                        val,
                        std::format("Value '{}' is not an allowed choice", val)
                    });
                }
            }

            for (const auto& v : arg->validators()) {
                auto v_res = v(val);
                if (!v_res) {
                    return std::unexpected(parse_error{
                        error_code::custom_validation_failed,
                        arg->name(),
                        val,
                        v_res.error()
                    });
                }
            }

            if (arg->get_action() == action::append) {
                result.append_value(arg->name(), std::string(val), true);
            } else {
                result.set_value(arg->name(), std::string(val), true);
            }
            return {};
        };

        while (i < raw_args.size()) {
            std::string_view current = raw_args[i];

            if (treat_all_as_positional) {
                if (positional_idx < positional_args.size()) {
                    const auto* p_arg = positional_args[positional_idx];
                    auto val_res = validate_and_store(p_arg, current);
                    if (!val_res) return std::unexpected(val_res.error());
                    if (p_arg->get_action() != action::append) {
                        positional_idx++;
                    }
                } else {
                    result.add_positional(std::string(current));
                }
                i++;
                continue;
            }

            if (current == "--") {
                treat_all_as_positional = true;
                i++;
                continue;
            }

            token tok = tokenizer::tokenize(current);

            if (tok.type == token_type::positional) {
                if (positional_idx < positional_args.size()) {
                    const auto* p_arg = positional_args[positional_idx];
                    auto val_res = validate_and_store(p_arg, current);
                    if (!val_res) return std::unexpected(val_res.error());
                    if (p_arg->get_action() != action::append) {
                        positional_idx++;
                    }
                } else {
                    result.add_positional(std::string(current));
                }
                i++;
            } else if (tok.type == token_type::long_option) {
                auto it = opt_map.find(tok.name);
                if (it == opt_map.end()) {
                    return std::unexpected(parse_error{
                        error_code::unknown_option,
                        tok.name,
                        tok.raw,
                        std::format("Unknown option '{}'", tok.name)
                    });
                }

                const argument* arg = it->second;
                auto grp_res = check_and_record_group(arg, tok.raw);
                if (!grp_res) return std::unexpected(grp_res.error());

                if (arg->is_flag()) {
                    if (tok.inline_value) {
                        auto val_res = validate_and_store(arg, *tok.inline_value);
                        if (!val_res) return std::unexpected(val_res.error());
                    } else {
                        std::string flag_val = arg->implicit_value().value_or(
                            arg->get_action() == action::store_true ? "true" : "false");
                        auto val_res = validate_and_store(arg, flag_val);
                        if (!val_res) return std::unexpected(val_res.error());
                    }
                    i++;
                } else if (arg->get_action() == action::count) {
                    size_t curr = result.count(arg->name());
                    result.set_value(arg->name(), std::format("{}", curr + 1), true);
                    i++;
                } else {
                    // Argument takes a value
                    std::string_view val;
                    if (tok.inline_value) {
                        val = *tok.inline_value;
                        i++;
                    } else {
                        if (i + 1 >= raw_args.size()) {
                            return std::unexpected(parse_error{
                                error_code::missing_value,
                                arg->name(),
                                tok.raw,
                                std::format("Option '{}' requires an argument", arg->name())
                            });
                        }
                        val = raw_args[i + 1];
                        i += 2;
                    }
                    auto val_res = validate_and_store(arg, val);
                    if (!val_res) return std::unexpected(val_res.error());
                }
            } else if (tok.type == token_type::short_option) {
                if (tok.inline_value) {
                    // e.g. -p=8080
                    auto it = opt_map.find(tok.name);
                    if (it == opt_map.end()) {
                        return std::unexpected(parse_error{
                            error_code::unknown_option,
                            tok.name,
                            tok.raw,
                            std::format("Unknown option '{}'", tok.name)
                        });
                    }
                    const argument* arg = it->second;
                    auto grp_res = check_and_record_group(arg, tok.raw);
                    if (!grp_res) return std::unexpected(grp_res.error());

                    auto val_res = validate_and_store(arg, *tok.inline_value);
                    if (!val_res) return std::unexpected(val_res.error());
                    i++;
                } else if (current.size() == 2) {
                    // Single short option, e.g. -v or -p
                    auto it = opt_map.find(current);
                    if (it == opt_map.end()) {
                        return std::unexpected(parse_error{
                            error_code::unknown_option,
                            current,
                            current,
                            std::format("Unknown option '{}'", current)
                        });
                    }

                    const argument* arg = it->second;
                    auto grp_res = check_and_record_group(arg, current);
                    if (!grp_res) return std::unexpected(grp_res.error());

                    if (arg->is_flag()) {
                        std::string flag_val = arg->implicit_value().value_or(
                            arg->get_action() == action::store_true ? "true" : "false");
                        auto val_res = validate_and_store(arg, flag_val);
                        if (!val_res) return std::unexpected(val_res.error());
                        i++;
                    } else if (arg->get_action() == action::count) {
                        size_t curr = result.count(arg->name());
                        result.set_value(arg->name(), std::format("{}", curr + 1), true);
                        i++;
                    } else {
                        // Takes a value
                        if (i + 1 >= raw_args.size()) {
                            return std::unexpected(parse_error{
                                error_code::missing_value,
                                arg->name(),
                                current,
                                std::format("Option '{}' requires an argument", arg->name())
                            });
                        }
                        std::string_view val = raw_args[i + 1];
                        auto val_res = validate_and_store(arg, val);
                        if (!val_res) return std::unexpected(val_res.error());
                        i += 2;
                    }
                } else {
                    // Short option chaining (e.g. -xvf) or inline value (e.g. -p8080)
                    std::string first_opt = std::format("-{}", current[1]);
                    auto it = opt_map.find(first_opt);
                    if (it == opt_map.end()) {
                        return std::unexpected(parse_error{
                            error_code::unknown_option,
                            first_opt,
                            current,
                            std::format("Unknown option '{}'", first_opt)
                        });
                    }

                    const argument* first_arg = it->second;
                    if (first_arg->takes_value()) {
                        // Inline value: -p8080 -> -p with value 8080
                        auto grp_res = check_and_record_group(first_arg, current);
                        if (!grp_res) return std::unexpected(grp_res.error());

                        std::string_view val = current.substr(2);
                        auto val_res = validate_and_store(first_arg, val);
                        if (!val_res) return std::unexpected(val_res.error());
                        i++;
                    } else {
                        // Flag chaining: -xvf
                        for (size_t c = 1; c < current.size(); ++c) {
                            std::string opt_char = std::format("-{}", current[c]);
                            auto opt_it = opt_map.find(opt_char);
                            if (opt_it == opt_map.end()) {
                                return std::unexpected(parse_error{
                                    error_code::unknown_option,
                                    opt_char,
                                    current,
                                    std::format("Unknown option '{}' in flag chain", opt_char)
                                });
                            }

                            const argument* chained_arg = opt_it->second;
                            auto grp_res = check_and_record_group(chained_arg, current);
                            if (!grp_res) return std::unexpected(grp_res.error());

                            if (chained_arg->is_flag()) {
                                std::string flag_val = chained_arg->implicit_value().value_or(
                                    chained_arg->get_action() == action::store_true ? "true" : "false");
                                auto val_res = validate_and_store(chained_arg, flag_val);
                                if (!val_res) return std::unexpected(val_res.error());
                            } else if (chained_arg->get_action() == action::count) {
                                size_t curr = result.count(chained_arg->name());
                                result.set_value(chained_arg->name(), std::format("{}", curr + 1), true);
                            } else if (chained_arg->takes_value()) {
                                // Remaining chars in the chain form the value!
                                std::string_view val;
                                if (c + 1 < current.size()) {
                                    val = current.substr(c + 1);
                                } else {
                                    if (i + 1 >= raw_args.size()) {
                                        return std::unexpected(parse_error{
                                            error_code::missing_value,
                                            chained_arg->name(),
                                            current,
                                            std::format("Option '{}' requires an argument", chained_arg->name())
                                        });
                                    }
                                    val = raw_args[i + 1];
                                    i++; // consume next
                                }
                                auto val_res = validate_and_store(chained_arg, val);
                                if (!val_res) return std::unexpected(val_res.error());
                                break; // end chaining loop
                            }
                        }
                        i++;
                    }
                }
            }
        }

        // Post-parsing: apply defaults
        for (const auto& arg : arguments) {
            if (!result.has(arg.name())) {
                if (arg.default_value()) {
                    result.set_value(arg.name(), *arg.default_value(), false);
                }
            }
        }

        // If --help was requested, bypass required constraint checks
        bool help_requested = result.has("--help") || result.has("-h");
        if (help_requested) {
            return result;
        }

        for (const auto& arg : arguments) {
            if (!result.has(arg.name()) && arg.is_required()) {
                return std::unexpected(parse_error{
                    error_code::missing_required_argument,
                    arg.name(),
                    "",
                    std::format("Required argument '{}' was not provided", arg.name())
                });
            }
        }

        // Check required mutually exclusive groups
        for (const auto& group : groups) {
            if (group.is_mutually_exclusive() && group.is_required()) {
                if (!group_first_seen_arg.contains(group.id())) {
                    return std::unexpected(parse_error{
                        error_code::missing_required_argument,
                        "",
                        "",
                        "One of the mutually exclusive arguments must be provided"
                    });
                }
            }
        }

        return result;
    }
};

} // namespace argparse
