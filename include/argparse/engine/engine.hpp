#pragma once

#include <argparse/compat/detect.hpp>
#include <argparse/compat/expected.hpp>
#include <argparse/compat/span.hpp>
#include <argparse/compat/string_view.hpp>
#include <argparse/config/argument.hpp>
#include <argparse/config/argument_group.hpp>
#include <argparse/core/error.hpp>
#include <argparse/engine/parse_result.hpp>
#include <argparse/engine/tokenizer.hpp>

#include <algorithm>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#if ARGPARSE_HAS_STD_FORMAT
    #include <format>
#endif

namespace argparse {

class engine {
public:
    [[nodiscard]] static expected<parse_result, parse_error> parse(
        const std::vector<argument>& arguments,
        const std::vector<argument_group>& groups,
        span<const string_view> raw_args) {

        parse_result result;

        std::unordered_map<std::string, const argument*> opt_map;
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

        auto check_and_record_group = [&](const argument* arg, string_view token_raw) -> expected<void, parse_error> {
            if (arg->has_group()) {
                size_t gid = arg->get_group_id();
                if (gid < groups.size() && groups[gid].is_mutually_exclusive()) {
                    auto it = group_first_seen_arg.find(gid);
                    if (it != group_first_seen_arg.end() && it->second != arg->name()) {
#if ARGPARSE_HAS_STD_FORMAT
                        std::string msg = std::format("Argument '{}' conflicts with previously specified '{}'", arg->name(), it->second);
#else
                        std::string msg = "Argument '" + arg->name() + "' conflicts with previously specified '" + it->second + "'";
#endif
                        return unexpected<parse_error>(parse_error{
                            error_code::mutually_exclusive_conflict,
                            arg->name(),
                            token_raw,
                            std::move(msg)
                        });
                    }
                    group_first_seen_arg[gid] = arg->name();
                }
            }
            return {};
        };

        auto validate_and_store = [&](const argument* arg, string_view val) -> expected<void, parse_error> {
            if (!arg->get_choices().empty()) {
                bool found = false;
                for (const auto& choice : arg->get_choices()) {
                    if (string_view(choice) == val) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
#if ARGPARSE_HAS_STD_FORMAT
                    std::string msg = std::format("Value '{}' is not an allowed choice", std::string_view(val.data(), val.size()));
#else
                    std::string msg = "Value '" + std::string(val.data(), val.size()) + "' is not an allowed choice";
#endif
                    return unexpected<parse_error>(parse_error{
                        error_code::choice_not_allowed,
                        arg->name(),
                        val,
                        std::move(msg)
                    });
                }
            }

            for (const auto& v : arg->validators()) {
                auto v_res = v(val);
                if (!v_res) {
                    return unexpected<parse_error>(parse_error{
                        error_code::custom_validation_failed,
                        arg->name(),
                        val,
                        v_res.error()
                    });
                }
            }

            if (arg->get_action() == action::append) {
                result.append_value(arg->name(), std::string(val.data(), val.size()), true);
            } else {
                result.set_value(arg->name(), std::string(val.data(), val.size()), true);
            }
            return {};
        };

        while (i < raw_args.size()) {
            string_view current = raw_args[i];

            if (treat_all_as_positional) {
                if (positional_idx < positional_args.size()) {
                    const auto* p_arg = positional_args[positional_idx];
                    auto val_res = validate_and_store(p_arg, current);
                    if (!val_res) return unexpected<parse_error>(val_res.error());
                    if (p_arg->get_action() != action::append) {
                        positional_idx++;
                    }
                } else {
                    result.add_positional(std::string(current.data(), current.size()));
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
                    if (!val_res) return unexpected<parse_error>(val_res.error());
                    if (p_arg->get_action() != action::append) {
                        positional_idx++;
                    }
                } else {
                    result.add_positional(std::string(current.data(), current.size()));
                }
                i++;
            } else if (tok.type == token_type::long_option) {
                std::string tok_name(tok.name.data(), tok.name.size());
                auto it = opt_map.find(tok_name);
                if (it == opt_map.end()) {
#if ARGPARSE_HAS_STD_FORMAT
                    std::string msg = std::format("Unknown option '{}'", tok_name);
#else
                    std::string msg = "Unknown option '" + tok_name + "'";
#endif
                    return unexpected<parse_error>(parse_error{
                        error_code::unknown_option,
                        tok.name,
                        tok.raw,
                        std::move(msg)
                    });
                }

                const argument* arg = it->second;
                auto grp_res = check_and_record_group(arg, tok.raw);
                if (!grp_res) return unexpected<parse_error>(grp_res.error());

                if (arg->is_flag()) {
                    if (tok.has_inline_value) {
                        auto val_res = validate_and_store(arg, tok.inline_value);
                        if (!val_res) return unexpected<parse_error>(val_res.error());
                    } else {
                        std::string flag_val = arg->has_implicit() ? arg->implicit_value() :
                            (arg->get_action() == action::store_true ? "true" : "false");
                        auto val_res = validate_and_store(arg, string_view(flag_val));
                        if (!val_res) return unexpected<parse_error>(val_res.error());
                    }
                    i++;
                } else if (arg->get_action() == action::count) {
                    size_t curr = result.count(arg->name());
                    result.set_value(arg->name(), std::to_string(curr + 1), true);
                    i++;
                } else {
                    string_view val;
                    if (tok.has_inline_value) {
                        val = tok.inline_value;
                        i++;
                    } else {
                        if (i + 1 >= raw_args.size()) {
#if ARGPARSE_HAS_STD_FORMAT
                            std::string msg = std::format("Option '{}' requires an argument", arg->name());
#else
                            std::string msg = "Option '" + arg->name() + "' requires an argument";
#endif
                            return unexpected<parse_error>(parse_error{
                                error_code::missing_value,
                                arg->name(),
                                tok.raw,
                                std::move(msg)
                            });
                        }
                        val = raw_args[i + 1];
                        i += 2;
                    }
                    auto val_res = validate_and_store(arg, val);
                    if (!val_res) return unexpected<parse_error>(val_res.error());
                }
            } else if (tok.type == token_type::short_option) {
                if (tok.has_inline_value) {
                    std::string tok_name(tok.name.data(), tok.name.size());
                    auto it = opt_map.find(tok_name);
                    if (it == opt_map.end()) {
#if ARGPARSE_HAS_STD_FORMAT
                        std::string msg = std::format("Unknown option '{}'", tok_name);
#else
                        std::string msg = "Unknown option '" + tok_name + "'";
#endif
                        return unexpected<parse_error>(parse_error{
                            error_code::unknown_option,
                            tok.name,
                            tok.raw,
                            std::move(msg)
                        });
                    }
                    const argument* arg = it->second;
                    auto grp_res = check_and_record_group(arg, tok.raw);
                    if (!grp_res) return unexpected<parse_error>(grp_res.error());

                    auto val_res = validate_and_store(arg, tok.inline_value);
                    if (!val_res) return unexpected<parse_error>(val_res.error());
                    i++;
                } else if (current.size() == 2) {
                    std::string cur_str(current.data(), current.size());
                    auto it = opt_map.find(cur_str);
                    if (it == opt_map.end()) {
#if ARGPARSE_HAS_STD_FORMAT
                        std::string msg = std::format("Unknown option '{}'", cur_str);
#else
                        std::string msg = "Unknown option '" + cur_str + "'";
#endif
                        return unexpected<parse_error>(parse_error{
                            error_code::unknown_option,
                            current,
                            current,
                            std::move(msg)
                        });
                    }

                    const argument* arg = it->second;
                    auto grp_res = check_and_record_group(arg, current);
                    if (!grp_res) return unexpected<parse_error>(grp_res.error());

                    if (arg->is_flag()) {
                        std::string flag_val = arg->has_implicit() ? arg->implicit_value() :
                            (arg->get_action() == action::store_true ? "true" : "false");
                        auto val_res = validate_and_store(arg, string_view(flag_val));
                        if (!val_res) return unexpected<parse_error>(val_res.error());
                        i++;
                    } else if (arg->get_action() == action::count) {
                        size_t curr = result.count(arg->name());
                        result.set_value(arg->name(), std::to_string(curr + 1), true);
                        i++;
                    } else {
                        if (i + 1 >= raw_args.size()) {
#if ARGPARSE_HAS_STD_FORMAT
                            std::string msg = std::format("Option '{}' requires an argument", arg->name());
#else
                            std::string msg = "Option '" + arg->name() + "' requires an argument";
#endif
                            return unexpected<parse_error>(parse_error{
                                error_code::missing_value,
                                arg->name(),
                                current,
                                std::move(msg)
                            });
                        }
                        string_view val = raw_args[i + 1];
                        auto val_res = validate_and_store(arg, val);
                        if (!val_res) return unexpected<parse_error>(val_res.error());
                        i += 2;
                    }
                } else {
                    std::string first_opt = std::string("-") + current[1];
                    auto it = opt_map.find(first_opt);
                    if (it == opt_map.end()) {
#if ARGPARSE_HAS_STD_FORMAT
                        std::string msg = std::format("Unknown option '{}'", first_opt);
#else
                        std::string msg = "Unknown option '" + first_opt + "'";
#endif
                        return unexpected<parse_error>(parse_error{
                            error_code::unknown_option,
                            current,
                            current,
                            std::move(msg)
                        });
                    }

                    const argument* first_arg = it->second;
                    if (first_arg->takes_value()) {
                        auto grp_res = check_and_record_group(first_arg, current);
                        if (!grp_res) return unexpected<parse_error>(grp_res.error());

                        string_view val = current.substr(2);
                        auto val_res = validate_and_store(first_arg, val);
                        if (!val_res) return unexpected<parse_error>(val_res.error());
                        i++;
                    } else {
                        for (size_t c = 1; c < current.size(); ++c) {
                            std::string opt_char = std::string("-") + current[c];
                            auto opt_it = opt_map.find(opt_char);
                            if (opt_it == opt_map.end()) {
#if ARGPARSE_HAS_STD_FORMAT
                                std::string msg = std::format("Unknown option '{}' in flag chain", opt_char);
#else
                                std::string msg = "Unknown option '" + opt_char + "' in flag chain";
#endif
                                return unexpected<parse_error>(parse_error{
                                    error_code::unknown_option,
                                    current,
                                    current,
                                    std::move(msg)
                                });
                            }

                            const argument* chained_arg = opt_it->second;
                            auto grp_res = check_and_record_group(chained_arg, current);
                            if (!grp_res) return unexpected<parse_error>(grp_res.error());

                            if (chained_arg->is_flag()) {
                                std::string flag_val = chained_arg->has_implicit() ? chained_arg->implicit_value() :
                                    (chained_arg->get_action() == action::store_true ? "true" : "false");
                                auto val_res = validate_and_store(chained_arg, string_view(flag_val));
                                if (!val_res) return unexpected<parse_error>(val_res.error());
                            } else if (chained_arg->get_action() == action::count) {
                                size_t curr = result.count(chained_arg->name());
                                result.set_value(chained_arg->name(), std::to_string(curr + 1), true);
                            } else if (chained_arg->takes_value()) {
                                string_view val;
                                if (c + 1 < current.size()) {
                                    val = current.substr(c + 1);
                                } else {
                                    if (i + 1 >= raw_args.size()) {
#if ARGPARSE_HAS_STD_FORMAT
                                        std::string msg = std::format("Option '{}' requires an argument", chained_arg->name());
#else
                                        std::string msg = "Option '" + chained_arg->name() + "' requires an argument";
#endif
                                        return unexpected<parse_error>(parse_error{
                                            error_code::missing_value,
                                            chained_arg->name(),
                                            current,
                                            std::move(msg)
                                        });
                                    }
                                    val = raw_args[i + 1];
                                    i++;
                                }
                                auto val_res = validate_and_store(chained_arg, val);
                                if (!val_res) return unexpected<parse_error>(val_res.error());
                                break;
                            }
                        }
                        i++;
                    }
                }
            }
        }

        for (const auto& arg : arguments) {
            if (!result.has(arg.name())) {
                if (arg.has_default()) {
                    result.set_value(arg.name(), arg.default_value(), false);
                }
            }
        }

        bool help_requested = result.has("--help") || result.has("-h");
        if (help_requested) {
            return result;
        }

        for (const auto& arg : arguments) {
            if (!result.has(arg.name()) && arg.is_required()) {
#if ARGPARSE_HAS_STD_FORMAT
                std::string msg = std::format("Required argument '{}' was not provided", arg.name());
#else
                std::string msg = "Required argument '" + arg.name() + "' was not provided";
#endif
                return unexpected<parse_error>(parse_error{
                    error_code::missing_required_argument,
                    arg.name(),
                    "",
                    std::move(msg)
                });
            }
        }

        for (const auto& group : groups) {
            if (group.is_mutually_exclusive() && group.is_required()) {
                if (group_first_seen_arg.find(group.id()) == group_first_seen_arg.end()) {
                    return unexpected<parse_error>(parse_error{
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
