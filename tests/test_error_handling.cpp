#include "test_framework.hpp"

#include <argparse/argparse.hpp>

TEST_CASE(test_unknown_option) {
    argparse::argument_parser parser("test");
    parser.add_argument("--known");

    std::vector<argparse::string_view> args = {"--unknown"};
    auto res = parser.parse_args(args);

    ASSERT_FALSE(res.has_value());
    ASSERT_EQ(res.error().code, argparse::error_code::unknown_option);
    ASSERT_EQ(res.error().token, "--unknown");
}

TEST_CASE(test_missing_value) {
    argparse::argument_parser parser("test");
    parser.add_argument("--output", "-o");

    std::vector<argparse::string_view> args = {"--output"};
    auto res = parser.parse_args(args);

    ASSERT_FALSE(res.has_value());
    ASSERT_EQ(res.error().code, argparse::error_code::missing_value);
}

TEST_CASE(test_monadic_operations) {
    argparse::argument_parser parser("test");
    parser.add_argument("--count").default_value<int32_t>(5);

    std::vector<argparse::string_view> args = {"--count", "10"};

    // Test .transform()
    auto doubled = parser.parse_args(args).transform(
        [](const argparse::parse_result &r) { return r.get<int32_t>("--count") * 2; });

    ASSERT_TRUE(doubled.has_value());
    ASSERT_EQ(*doubled, 20);

    // Test .and_then()
    auto validated = parser.parse_args(args).and_then(
        [](const argparse::parse_result &r) -> argparse::expected<int32_t, argparse::parse_error> {
            int32_t val = r.get<int32_t>("--count");
            if (val > 100) {
                return argparse::unexpected<argparse::parse_error>(
                    argparse::parse_error{argparse::error_code::invalid_value, "--count", "", "Count too high"});
            }
            return val;
        });

    ASSERT_TRUE(validated.has_value());
    ASSERT_EQ(*validated, 10);
}

TEST_CASE(test_pure_functional_immutability) {
    argparse::argument_parser parser("test");
    parser.add_argument("--name");
    parser.add_argument("-v").flag();

    std::vector<argparse::string_view> args1 = {"--name", "alpha", "-v"};
    std::vector<argparse::string_view> args2 = {"--name", "beta"};

    auto res1 = parser.parse_args(args1);
    auto res2 = parser.parse_args(args2);

    ASSERT_TRUE(res1.has_value());
    ASSERT_TRUE(res2.has_value());

    // Verify res1 and res2 do not leak state to each other
    ASSERT_EQ((*res1).get<std::string>("--name"), "alpha");
    ASSERT_TRUE((*res1).get<bool>("-v"));

    ASSERT_EQ((*res2).get<std::string>("--name"), "beta");
    ASSERT_FALSE((*res2).has("-v"));
}

int main() {
    return test_framework::run_all();
}
