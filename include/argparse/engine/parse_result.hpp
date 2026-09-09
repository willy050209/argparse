#pragma once

#include <argparse/compat/detect.hpp>
#include <argparse/compat/expected.hpp>
#include <argparse/compat/span.hpp>
#include <argparse/compat/string_view.hpp>
#include <argparse/compat/traits.hpp>
#include <argparse/core/error.hpp>
#include <argparse/core/value_parser.hpp>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#if ARGPARSE_HAS_STD_FORMAT
#include <format>
#endif

namespace argparse {

namespace detail {
template <typename T>
struct try_get_helper {
    static expected<T, parse_error> parse(const std::vector<std::string> &vals, string_view) {
        return value_parser<T>::parse(string_view(vals.back()));
    }
};

template <typename T, typename Alloc>
struct try_get_helper<std::vector<T, Alloc>> {
    using VecT = std::vector<T, Alloc>;
    static expected<VecT, parse_error> parse(const std::vector<std::string> &vals, string_view) {
        VecT vec;
        for (const auto &raw_val : vals) {
            auto parsed_sub = value_parser<VecT>::parse(string_view(raw_val));
            if (parsed_sub) {
                for (auto &&item : *parsed_sub) {
                    vec.push_back(std::move(item));
                }
            } else {
                auto single_item = value_parser<T>::parse(string_view(raw_val));
                if (single_item) {
                    vec.push_back(std::move(*single_item));
                }
            }
        }
        return vec;
    }
};
} // namespace detail

class parse_result {
public:
    parse_result() = default;

    [[nodiscard]] bool has(string_view name) const noexcept {
        auto canonical = resolve_canonical_name(name);
        auto it = m_values.find(std::string(canonical.data(), canonical.size()));
        if (it == m_values.end() || it->second.empty()) {
            return false;
        }
        if (it->second.size() == 1 && it->second.front() == "false") {
            return m_explicitly_present.count(std::string(canonical.data(), canonical.size())) > 0;
        }
        return true;
    }

    [[nodiscard]] bool is_explicit(string_view name) const noexcept {
        auto canonical = resolve_canonical_name(name);
        return m_explicitly_present.count(std::string(canonical.data(), canonical.size())) > 0;
    }

    [[nodiscard]] size_t count(string_view name) const noexcept {
        auto canonical = resolve_canonical_name(name);
        auto it = m_values.find(std::string(canonical.data(), canonical.size()));
        if (it == m_values.end()) {
            return 0;
        }
        if (it->second.size() == 1) {
            auto parsed = value_parser<size_t>::parse(string_view(it->second.front()));
            if (parsed) {
                return *parsed;
            }
        }
        return it->second.size();
    }

    [[nodiscard]] bool has_raw(string_view name) const noexcept {
        auto canonical = resolve_canonical_name(name);
        auto it = m_values.find(std::string(canonical.data(), canonical.size()));
        return it != m_values.end() && !it->second.empty();
    }

    [[nodiscard]] string_view get_raw(string_view name) const noexcept {
        auto canonical = resolve_canonical_name(name);
        auto it = m_values.find(std::string(canonical.data(), canonical.size()));
        if (it == m_values.end() || it->second.empty()) {
            return string_view{};
        }
        return string_view(it->second.back());
    }

    [[nodiscard]] std::vector<string_view> get_raw_list(string_view name) const {
        std::vector<string_view> result;
        auto canonical = resolve_canonical_name(name);
        auto it = m_values.find(std::string(canonical.data(), canonical.size()));
        if (it != m_values.end()) {
            result.reserve(it->second.size());
            for (const auto &s : it->second) {
                result.emplace_back(s.data(), s.size());
            }
        }
        return result;
    }

    template <typename T>
    [[nodiscard]] expected<T, parse_error> try_get(string_view name) const noexcept {
        auto canonical = resolve_canonical_name(name);
        auto it = m_values.find(std::string(canonical.data(), canonical.size()));
        if (it == m_values.end() || it->second.empty()) {
            return unexpected<parse_error>(parse_error{error_code::missing_value, name, "", "Value not found"});
        }

        return detail::try_get_helper<T>::parse(it->second, name);
    }

    template <typename T>
    [[nodiscard]] T get(string_view name) const {
        auto val = try_get<T>(name);
        if (!val) {
#if ARGPARSE_HAS_STD_FORMAT
            throw std::runtime_error(std::format("Argument '{}' not found or failed to convert",
                                                 std::string_view(name.data(), name.size())));
#else
            std::ostringstream oss;
            oss << "Argument '" << name << "' not found or failed to convert";
            throw std::runtime_error(oss.str());
#endif
        }
        return std::move(*val);
    }

    template <typename T>
    [[nodiscard]] T get_or(string_view name, T fallback) const noexcept {
        auto val = try_get<T>(name);
        if (val) {
            return std::move(*val);
        }
        return fallback;
    }

    [[nodiscard]] const std::vector<std::string> &positionals() const noexcept { return m_positionals; }

    void set_value(string_view canonical_name, std::string value, bool is_explicit = true) {
        std::string name_str(canonical_name.data(), canonical_name.size());
        m_values[name_str] = {std::move(value)};
        if (is_explicit) {
            m_explicitly_present.insert(name_str);
        }
    }

    void append_value(string_view canonical_name, std::string value, bool is_explicit = true) {
        std::string name_str(canonical_name.data(), canonical_name.size());
        m_values[name_str].push_back(std::move(value));
        if (is_explicit) {
            m_explicitly_present.insert(name_str);
        }
    }

    void add_positional(std::string value) { m_positionals.push_back(std::move(value)); }

    void register_alias(string_view alias, string_view canonical_name) {
        m_alias_map[std::string(alias.data(), alias.size())] =
            std::string(canonical_name.data(), canonical_name.size());
    }

private:
    [[nodiscard]] string_view resolve_canonical_name(string_view name) const noexcept {
        auto it = m_alias_map.find(std::string(name.data(), name.size()));
        if (it != m_alias_map.end()) {
            return string_view(it->second);
        }
        return name;
    }

    std::unordered_map<std::string, std::vector<std::string>> m_values;
    std::unordered_map<std::string, std::string> m_alias_map;
    std::unordered_set<std::string> m_explicitly_present;
    std::vector<std::string> m_positionals;
};

} // namespace argparse
