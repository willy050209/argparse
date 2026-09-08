#pragma once

#include <argparse/core/error.hpp>
#include <argparse/core/traits.hpp>
#include <argparse/core/value_parser.hpp>

#include <cstdint>
#include <expected>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace argparse {

/**
 * @brief Immutable, type-safe argument parsing result container.
 */
class parse_result {
public:
    parse_result() = default;

    /**
     * @brief Check if an option or flag was provided on the command line or has a default.
     */
    [[nodiscard]] bool has(std::string_view name) const noexcept {
        auto canonical = resolve_canonical_name(name);
        auto it = m_values.find(std::string(canonical));
        if (it == m_values.end() || it->second.empty()) {
            return false;
        }
        // If it's a flag with value "false", has() returns false if user didn't explicitly specify it
        if (it->second.size() == 1 && it->second.front() == "false") {
            return m_explicitly_present.contains(std::string(canonical));
        }
        return true;
    }

    /**
     * @brief Check if argument was explicitly provided by the user (ignoring defaults).
     */
    [[nodiscard]] bool is_explicit(std::string_view name) const noexcept {
        auto canonical = resolve_canonical_name(name);
        return m_explicitly_present.contains(std::string(canonical));
    }

    /**
     * @brief Number of occurrences of a flag or option.
     */
    [[nodiscard]] size_t count(std::string_view name) const noexcept {
        auto canonical = resolve_canonical_name(name);
        auto it = m_values.find(std::string(canonical));
        if (it == m_values.end()) {
            return 0;
        }
        // If it's a counter action stored as integer string
        if (it->second.size() == 1) {
            auto parsed = value_parser<size_t>::parse(it->second.front());
            if (parsed) {
                return *parsed;
            }
        }
        return it->second.size();
    }

    /**
     * @brief Get raw string value of an argument.
     */
    [[nodiscard]] std::optional<std::string_view> get_raw(std::string_view name) const noexcept {
        auto canonical = resolve_canonical_name(name);
        auto it = m_values.find(std::string(canonical));
        if (it == m_values.end() || it->second.empty()) {
            return std::nullopt;
        }
        return it->second.back();
    }

    /**
     * @brief Get all raw values associated with an argument (e.g. for append action).
     */
    [[nodiscard]] std::vector<std::string_view> get_raw_list(std::string_view name) const {
        std::vector<std::string_view> result;
        auto canonical = resolve_canonical_name(name);
        auto it = m_values.find(std::string(canonical));
        if (it != m_values.end()) {
            result.reserve(it->second.size());
            for (const auto& s : it->second) {
                result.emplace_back(s);
            }
        }
        return result;
    }

    /**
     * @brief Try to get and convert value to type T without throwing exceptions.
     */
    template <parsable T>
    [[nodiscard]] std::optional<T> try_get(std::string_view name) const noexcept {
        auto canonical = resolve_canonical_name(name);
        auto it = m_values.find(std::string(canonical));
        if (it == m_values.end() || it->second.empty()) {
            return std::nullopt;
        }

        if constexpr (is_vector_v<T>) {
            // For vector types, if multiple values were recorded, reconstruct
            using ItemT = typename T::value_type;
            T vec;
            for (const auto& raw_val : it->second) {
                // If the single item itself is comma-separated, parse with value_parser<std::vector<ItemT>>
                auto parsed_sub = value_parser<T>::parse(raw_val);
                if (parsed_sub) {
                    for (auto&& item : *parsed_sub) {
                        vec.push_back(std::move(item));
                    }
                } else {
                    auto single_item = value_parser<ItemT>::parse(raw_val);
                    if (single_item) {
                        vec.push_back(std::move(*single_item));
                    }
                }
            }
            return vec;
        } else {
            auto parsed = value_parser<T>::parse(it->second.back());
            if (parsed) {
                return *parsed;
            }
            return std::nullopt;
        }
    }

    /**
     * @brief Strongly-typed access to argument value.
     */
    template <parsable T>
    [[nodiscard]] T get(std::string_view name) const {
        auto val = try_get<T>(name);
        if (!val) {
            throw std::runtime_error(std::format("Argument '{}' not found or failed to convert to target type", name));
        }
        return std::move(*val);
    }

    /**
     * @brief Strongly-typed access with fallback default value.
     */
    template <parsable T>
    [[nodiscard]] T get_or(std::string_view name, T fallback) const noexcept {
        auto val = try_get<T>(name);
        if (val) {
            return std::move(*val);
        }
        return fallback;
    }

    /**
     * @brief Get positional arguments in order of appearance.
     */
    [[nodiscard]] const std::vector<std::string>& positionals() const noexcept {
        return m_positionals;
    }

    // Builder methods (used by parsing engine)
    void set_value(std::string_view canonical_name, std::string value, bool is_explicit = true) {
        std::string name_str(canonical_name);
        m_values[name_str] = {std::move(value)};
        if (is_explicit) {
            m_explicitly_present.insert(name_str);
        }
    }

    void append_value(std::string_view canonical_name, std::string value, bool is_explicit = true) {
        std::string name_str(canonical_name);
        m_values[name_str].push_back(std::move(value));
        if (is_explicit) {
            m_explicitly_present.insert(name_str);
        }
    }

    void add_positional(std::string value) {
        m_positionals.push_back(std::move(value));
    }

    void register_alias(std::string_view alias, std::string_view canonical_name) {
        m_alias_map[std::string(alias)] = std::string(canonical_name);
    }

private:
    [[nodiscard]] std::string_view resolve_canonical_name(std::string_view name) const noexcept {
        auto it = m_alias_map.find(std::string(name));
        if (it != m_alias_map.end()) {
            return it->second;
        }
        return name;
    }

    std::unordered_map<std::string, std::vector<std::string>> m_values;
    std::unordered_map<std::string, std::string> m_alias_map;
    std::unordered_set<std::string> m_explicitly_present;
    std::vector<std::string> m_positionals;
};

} // namespace argparse
