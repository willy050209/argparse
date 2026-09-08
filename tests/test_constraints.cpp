#include "test_framework.hpp"
#include <argparse/argparse.hpp>

TEST_CASE(test_required_argument_missing) {
    argparse::argument_parser parser("test");
    parser.add_argument("--config", "-c").required(true);

    std::vector<argparse::string_view> args = {};
    auto res = parser.parse_args(args);

    ASSERT_FALSE(res.has_value());
    ASSERT_EQ(res.error().code, argparse::error_code::missing_required_argument);
}

TEST_CASE(test_choices_validation) {
    argparse::argument_parser parser("test");
    parser.add_argument("--format").choices("json", "xml", "yaml");

    // Valid choice
    {
        std::vector<argparse::string_view> valid_args = { "--format", "json" };
        auto res = parser.parse_args(valid_args);
        ASSERT_TRUE(res.has_value());
        ASSERT_EQ((*res).get<std::string>("--format"), "json");
    }

    // Invalid choice
    {
        std::vector<argparse::string_view> invalid_args = { "--format", "binary" };
        auto res = parser.parse_args(invalid_args);
        ASSERT_FALSE(res.has_value());
        ASSERT_EQ(res.error().code, argparse::error_code::choice_not_allowed);
    }
}

TEST_CASE(test_mutually_exclusive_group_conflict) {
    argparse::argument_parser parser("test");
    auto& group = parser.add_mutually_exclusive_group();
    group.add_argument("--tcp").flag();
    group.add_argument("--udp").flag();

    std::vector<argparse::string_view> args = { "--tcp", "--udp" };
    auto res = parser.parse_args(args);

    ASSERT_FALSE(res.has_value());
    ASSERT_EQ(res.error().code, argparse::error_code::mutually_exclusive_conflict);
}

TEST_CASE(test_mutually_exclusive_group_required) {
    argparse::argument_parser parser("test");
    auto& group = parser.add_mutually_exclusive_group(true);
    group.add_argument("--client").flag();
    group.add_argument("--server").flag();

    // Neither provided
    {
        std::vector<argparse::string_view> args = {};
        auto res = parser.parse_args(args);
        ASSERT_FALSE(res.has_value());
        ASSERT_EQ(res.error().code, argparse::error_code::missing_required_argument);
    }

    // Exactly one provided
    {
        std::vector<argparse::string_view> args = { "--server" };
        auto res = parser.parse_args(args);
        ASSERT_TRUE(res.has_value());
        ASSERT_TRUE((*res).get<bool>("--server"));
        ASSERT_FALSE((*res).has("--client"));
    }
}

TEST_CASE(test_custom_typed_validator) {
    argparse::argument_parser parser("test");
    parser.add_argument("--port", "-p")
        .default_value<int32_t>(8080)
        .validator<int32_t>([](int32_t p) {
            return p >= 1024 && p <= 65535;
        }, "Port must be in unprivileged range 1024-65535");

    // Valid
    {
        std::vector<argparse::string_view> args = { "-p", "3000" };
        auto res = parser.parse_args(args);
        ASSERT_TRUE(res.has_value());
        ASSERT_EQ((*res).get<int32_t>("-p"), 3000);
    }

    // Invalid (privileged port)
    {
        std::vector<argparse::string_view> args = { "-p", "80" };
        auto res = parser.parse_args(args);
        ASSERT_FALSE(res.has_value());
        ASSERT_EQ(res.error().code, argparse::error_code::custom_validation_failed);
    }
}

TEST_CASE(test_defaults_and_implicit_values) {
    argparse::argument_parser parser("test");
    parser.add_argument("--timeout").default_value<int32_t>(30);
    parser.add_argument("--color").implicit_value(std::string("always")).default_value(std::string("auto"));

    // Both omitted: defaults take effect
    {
        std::vector<argparse::string_view> args = {};
        auto res = parser.parse_args(args);
        ASSERT_TRUE(res.has_value());
        ASSERT_EQ((*res).get<int32_t>("--timeout"), 30);
        ASSERT_EQ((*res).get<std::string>("--color"), "auto");
        ASSERT_FALSE((*res).is_explicit("--color"));
    }

    // Explicitly provided with value
    {
        std::vector<argparse::string_view> args = { "--color", "never" };
        auto res = parser.parse_args(args);
        ASSERT_TRUE(res.has_value());
        ASSERT_EQ((*res).get<std::string>("--color"), "never");
        ASSERT_TRUE((*res).is_explicit("--color"));
    }
}

int main() {
    return test_framework::run_all();
}
