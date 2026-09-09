#pragma once

#include <argparse/compat/detect.hpp>
#include <argparse/compat/expected.hpp>
#include <argparse/compat/span.hpp>
#include <argparse/compat/string_view.hpp>
#include <argparse/compat/traits.hpp>
#include <argparse/config/action.hpp>
#include <argparse/core/error.hpp>
#include <argparse/core/value_parser.hpp>
#include <cctype>
#include <functional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#if ARGPARSE_HAS_STD_FORMAT
#include <format>
#endif

namespace argparse {
namespace detail {
inline std::string to_string_repr(const std::string &val) {
    return val;
}
inline std::string to_string_repr(string_view val) {
    return std::string(val.data(), val.size());
}
inline std::string to_string_repr(const char *val) {
    return std::string(val ? val : "");
}
inline std::string to_string_repr(bool val) {
    return val ? "true" : "false";
}

template <typename T>
inline std::string to_string_repr(const T &val) {
#if ARGPARSE_HAS_STD_FORMAT
    return std::format("{}", val);
#else
    std::ostringstream oss;
    oss << val;
    return oss.str();
#endif
}
} // namespace detail

class argument {
public:
    using validator_fn = std::function<expected<void, std::string>(string_view)>;

    explicit argument(string_view name, string_view short_name = "")
        : m_name(name.data(), name.size()), m_short_name(short_name.data(), short_name.size()) {
        m_positional = !starts_with(name, '-');
        if (m_positional) {
            m_metavar = m_name;
        } else {
            string_view clean = name;
            while (starts_with(clean, '-')) {
                clean.remove_prefix(1);
            }
            m_metavar = std::string(clean.data(), clean.size());
            for (char &c : m_metavar) {
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                if (c == '-')
                    c = '_';
            }
        }
    }

    argument &help(std::string text) {
        m_help = std::move(text);
        return *this;
    }

    argument &metavar(std::string mv) {
        m_metavar = std::move(mv);
        return *this;
    }

    argument &required(bool req = true) noexcept {
        m_required = req;
        return *this;
    }

    argument &flag() noexcept {
        m_action = action::store_true;
        m_has_default = true;
        m_default_value = "false";
        m_has_implicit = true;
        m_implicit_value = "true";
        return *this;
    }

    argument &store_false() noexcept {
        m_action = action::store_false;
        m_has_default = true;
        m_default_value = "true";
        m_has_implicit = true;
        m_implicit_value = "false";
        return *this;
    }

    argument &count() noexcept {
        m_action = action::count;
        m_has_default = true;
        m_default_value = "0";
        return *this;
    }

    argument &append() {
        m_action = action::append;
        return *this;
    }

    template <typename T>
    argument &default_value(const T &val) {
        m_has_default = true;
        m_default_value = detail::to_string_repr(val);
        return *this;
    }

    template <typename T>
    argument &implicit_value(const T &val) {
        m_has_implicit = true;
        m_implicit_value = detail::to_string_repr(val);
        return *this;
    }

    template <typename... Args>
    argument &choices(Args &&...chs) {
        std::vector<std::string> items = {std::string(chs)...};
        for (auto &&item : items) {
            m_choices.push_back(std::move(item));
        }
        return *this;
    }

    argument &choices(span<const string_view> chs) {
        for (size_t i = 0; i < chs.size(); ++i) {
            m_choices.emplace_back(chs[i].data(), chs[i].size());
        }
        return *this;
    }

    argument &choices(const std::vector<std::string> &chs) {
        m_choices = chs;
        return *this;
    }

    template <typename T, typename F>
    argument &validator(F func, std::string error_msg = "Constraint validation failed") {
        m_validators.push_back([func, error_msg](string_view raw) -> expected<void, std::string> {
            auto parsed = value_parser<T>::parse(raw);
            if (!parsed) {
                return unexpected<std::string>(parsed.error().message);
            }
            if (!func(*parsed)) {
                return unexpected<std::string>(error_msg);
            }
            return {};
        });
        return *this;
    }

    argument &group_id(size_t gid) noexcept {
        m_has_group = true;
        m_group_id = gid;
        return *this;
    }

    // Accessors
    [[nodiscard]] const std::string &name() const noexcept { return m_name; }
    [[nodiscard]] const std::string &short_name() const noexcept { return m_short_name; }
    [[nodiscard]] const std::string &help() const noexcept { return m_help; }
    [[nodiscard]] const std::string &metavar() const noexcept { return m_metavar; }
    [[nodiscard]] action get_action() const noexcept { return m_action; }
    [[nodiscard]] bool is_required() const noexcept { return m_required; }
    [[nodiscard]] bool is_positional() const noexcept { return m_positional; }
    [[nodiscard]] bool is_flag() const noexcept {
        return m_action == action::store_true || m_action == action::store_false;
    }
    [[nodiscard]] bool takes_value() const noexcept { return m_action == action::store || m_action == action::append; }
    [[nodiscard]] bool has_default() const noexcept { return m_has_default; }
    [[nodiscard]] const std::string &default_value() const noexcept { return m_default_value; }
    [[nodiscard]] bool has_implicit() const noexcept { return m_has_implicit; }
    [[nodiscard]] const std::string &implicit_value() const noexcept { return m_implicit_value; }
    [[nodiscard]] const std::vector<std::string> &get_choices() const noexcept { return m_choices; }
    [[nodiscard]] const std::vector<validator_fn> &validators() const noexcept { return m_validators; }
    [[nodiscard]] bool has_group() const noexcept { return m_has_group; }
    [[nodiscard]] size_t get_group_id() const noexcept { return m_group_id; }

    [[nodiscard]] bool matches(string_view opt) const noexcept {
        return opt == string_view(m_name) || (!m_short_name.empty() && opt == string_view(m_short_name));
    }

private:
    std::string m_name;
    std::string m_short_name;
    std::string m_help;
    std::string m_metavar;
    action m_action{action::store};
    bool m_required{false};
    bool m_positional{false};
    bool m_has_default{false};
    std::string m_default_value;
    bool m_has_implicit{false};
    std::string m_implicit_value;
    std::vector<std::string> m_choices;
    std::vector<validator_fn> m_validators;
    bool m_has_group{false};
    size_t m_group_id{0};
};

} // namespace argparse
