#pragma once

#include <argparse/config/action.hpp>
#include <argparse/core/error.hpp>
#include <argparse/core/traits.hpp>
#include <argparse/core/value_parser.hpp>

#include <concepts>
#include <expected>
#include <format>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace argparse {

class argument {
public:
    using validator_fn = std::function<std::expected<void, std::string>(std::string_view)>;

    explicit argument(std::string_view name, std::string_view short_name = "")
        : m_name(name), m_short_name(short_name) {
        // Determine if positional: doesn't start with '-'
        m_positional = !m_name.starts_with('-');
        if (m_positional) {
            m_metavar = m_name;
        } else {
            // Default metavar derived from name
            std::string_view clean = m_name;
            while (clean.starts_with('-')) {
                clean.remove_prefix(1);
            }
            m_metavar = clean;
            for (char& c : m_metavar) {
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            }
        }
    }

    argument& help(std::string_view help_text) {
        m_help = std::string(help_text);
        return *this;
    }

    argument& metavar(std::string_view mv) {
        m_metavar = std::string(mv);
        return *this;
    }

    argument& required(bool req = true) {
        m_required = req;
        return *this;
    }

    argument& flag() {
        m_action = action::store_true;
        m_default_value = "false";
        m_implicit_value = "true";
        return *this;
    }

    argument& count() {
        m_action = action::count;
        m_default_value = "0";
        return *this;
    }

    argument& append() {
        m_action = action::append;
        return *this;
    }

    template <typename T>
    argument& default_value(const T& val) {
        if constexpr (std::same_as<T, std::string>) {
            m_default_value = val;
        } else if constexpr (std::same_as<T, std::string_view> || std::same_as<T, const char*>) {
            m_default_value = std::string(val);
        } else if constexpr (std::same_as<T, bool>) {
            m_default_value = val ? "true" : "false";
        } else {
            m_default_value = std::format("{}", val);
        }
        return *this;
    }

    template <typename T>
    argument& implicit_value(const T& val) {
        if constexpr (std::same_as<T, std::string>) {
            m_implicit_value = val;
        } else if constexpr (std::same_as<T, std::string_view> || std::same_as<T, const char*>) {
            m_implicit_value = std::string(val);
        } else if constexpr (std::same_as<T, bool>) {
            m_implicit_value = val ? "true" : "false";
        } else {
            m_implicit_value = std::format("{}", val);
        }
        return *this;
    }

    template <std::convertible_to<std::string_view>... Args>
    argument& choices(Args&&... chs) {
        (m_choices.emplace_back(std::forward<Args>(chs)), ...);
        return *this;
    }

    argument& choices(std::span<const std::string_view> chs) {
        for (auto sv : chs) {
            m_choices.emplace_back(sv);
        }
        return *this;
    }

    argument& choices(const std::vector<std::string>& chs) {
        m_choices = chs;
        return *this;
    }

    template <typename T, typename F>
        requires parsable<T> && validator_for<F, T>
    argument& validator(F&& func, std::string error_msg = "Constraint validation failed") {
        m_validators.push_back([f = std::forward<F>(func), msg = std::move(error_msg)](std::string_view raw) -> std::expected<void, std::string> {
            auto parsed = value_parser<T>::parse(raw);
            if (!parsed) {
                return std::unexpected(parsed.error().message);
            }
            if constexpr (result_validator_for<F, T>) {
                return f(*parsed);
            } else {
                if (!f(*parsed)) {
                    return std::unexpected(msg);
                }
                return {};
            }
        });
        return *this;
    }

    template <typename F>
        requires (!parsable<F>)
    argument& validator(F&& func, std::string error_msg = "Validation failed") {
        m_validators.push_back([f = std::forward<F>(func), msg = std::move(error_msg)](std::string_view raw) -> std::expected<void, std::string> {
            if (!f(raw)) {
                return std::unexpected(msg);
            }
            return {};
        });
        return *this;
    }

    argument& group_id(size_t gid) noexcept {
        m_group_id = gid;
        return *this;
    }

    // Accessors
    [[nodiscard]] const std::string& name() const noexcept { return m_name; }
    [[nodiscard]] const std::string& short_name() const noexcept { return m_short_name; }
    [[nodiscard]] const std::string& help() const noexcept { return m_help; }
    [[nodiscard]] const std::string& metavar() const noexcept { return m_metavar; }
    [[nodiscard]] action get_action() const noexcept { return m_action; }
    [[nodiscard]] bool is_required() const noexcept { return m_required; }
    [[nodiscard]] bool is_positional() const noexcept { return m_positional; }
    [[nodiscard]] bool is_flag() const noexcept { return m_action == action::store_true || m_action == action::store_false; }
    [[nodiscard]] bool takes_value() const noexcept { return m_action == action::store || m_action == action::append; }
    [[nodiscard]] const std::optional<std::string>& default_value() const noexcept { return m_default_value; }
    [[nodiscard]] const std::optional<std::string>& implicit_value() const noexcept { return m_implicit_value; }
    [[nodiscard]] const std::vector<std::string>& get_choices() const noexcept { return m_choices; }
    [[nodiscard]] const std::vector<validator_fn>& validators() const noexcept { return m_validators; }
    [[nodiscard]] std::optional<size_t> get_group_id() const noexcept { return m_group_id; }

    [[nodiscard]] bool matches(std::string_view opt) const noexcept {
        return opt == m_name || (!m_short_name.empty() && opt == m_short_name);
    }

private:
    std::string m_name;
    std::string m_short_name;
    std::string m_help;
    std::string m_metavar;
    action m_action{action::store};
    bool m_required{false};
    bool m_positional{false};
    std::optional<std::string> m_default_value;
    std::optional<std::string> m_implicit_value;
    std::vector<std::string> m_choices;
    std::vector<validator_fn> m_validators;
    std::optional<size_t> m_group_id;
};

} // namespace argparse
