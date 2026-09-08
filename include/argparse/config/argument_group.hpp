#pragma once

#include <argparse/config/argument.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace argparse {

// Forward declaration
class argument_parser;

class argument_group {
public:
    argument_group(size_t id, argument_parser& parent, bool mutually_exclusive = false, bool required = false)
        : m_id(id), m_parent(&parent), m_mutually_exclusive(mutually_exclusive), m_required(required) {}

    argument& add_argument(std::string_view name, std::string_view short_name = "");

    [[nodiscard]] size_t id() const noexcept { return m_id; }
    [[nodiscard]] bool is_mutually_exclusive() const noexcept { return m_mutually_exclusive; }
    [[nodiscard]] bool is_required() const noexcept { return m_required; }
    [[nodiscard]] const std::vector<std::string>& argument_names() const noexcept { return m_argument_names; }

    void register_argument_name(std::string_view name) {
        m_argument_names.emplace_back(name);
    }

private:
    size_t m_id;
    argument_parser* m_parent;
    bool m_mutually_exclusive{false};
    bool m_required{false};
    std::vector<std::string> m_argument_names;
};

} // namespace argparse
