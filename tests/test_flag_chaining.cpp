#include "test_framework.hpp"

#include <argparse/argparse.hpp>

TEST_CASE(test_short_flag_chaining) {
    argparse::argument_parser parser("tar");
    parser.add_argument("--extract", "-x").flag();
    parser.add_argument("--verbose", "-v").flag();
    parser.add_argument("--force", "-f").flag();

    std::vector<argparse::string_view> args = {"-xvf"};

    auto res = parser.parse_args(args);
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE((*res).get<bool>("-x"));
    ASSERT_TRUE((*res).get<bool>("-v"));
    ASSERT_TRUE((*res).get<bool>("-f"));
    ASSERT_TRUE((*res).has("--extract"));
    ASSERT_TRUE((*res).has("--verbose"));
    ASSERT_TRUE((*res).has("--force"));
}

TEST_CASE(test_flag_chaining_with_value_suffix) {
    argparse::argument_parser parser("tool");
    parser.add_argument("--all", "-a").flag();
    parser.add_argument("--long-list", "-l").flag();
    parser.add_argument("--output", "-o");

    std::vector<argparse::string_view> args = {"-alooutput.log"};

    auto res = parser.parse_args(args);
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE((*res).get<bool>("-a"));
    ASSERT_TRUE((*res).get<bool>("-l"));
    ASSERT_EQ((*res).get<std::string>("-o"), "output.log");
}

TEST_CASE(test_flag_chaining_with_next_arg_value) {
    argparse::argument_parser parser("tool");
    parser.add_argument("--all", "-a").flag();
    parser.add_argument("--output", "-o");

    std::vector<argparse::string_view> args = {"-ao", "target.bin"};

    auto res = parser.parse_args(args);
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE((*res).get<bool>("-a"));
    ASSERT_EQ((*res).get<std::string>("-o"), "target.bin");
}

TEST_CASE(test_count_flag) {
    argparse::argument_parser parser("test");
    parser.add_argument("--verbose", "-v").count();

    std::vector<argparse::string_view> args = {"-v", "-v", "-v"};

    auto res = parser.parse_args(args);
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ((*res).count("-v"), 3u);
}

TEST_CASE(test_inline_equals_syntax) {
    argparse::argument_parser parser("test");
    parser.add_argument("--output", "-o");
    parser.add_argument("--level", "-l").default_value<int32_t>(0);

    std::vector<argparse::string_view> args = {"--output=dist/bundle.js", "-l=4"};

    auto res = parser.parse_args(args);
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ((*res).get<std::string>("--output"), "dist/bundle.js");
    ASSERT_EQ((*res).get<int32_t>("-l"), 4);
}

TEST_CASE(test_delimiter_double_dash) {
    argparse::argument_parser parser("test");
    parser.add_argument("--flag", "-f").flag();
    parser.add_argument("cmd");

    std::vector<argparse::string_view> args = {"-f", "--", "--not-an-option", "-x"};

    auto res = parser.parse_args(args);
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE((*res).get<bool>("-f"));
    ASSERT_EQ((*res).get<std::string>("cmd"), "--not-an-option");

    const auto &extra = (*res).positionals();
    ASSERT_EQ(extra.size(), 1u);
    ASSERT_EQ(extra[0], "-x");
}

int main() {
    return test_framework::run_all();
}
