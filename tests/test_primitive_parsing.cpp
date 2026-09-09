#include "test_framework.hpp"

#include <argparse/argparse.hpp>

// Custom type for trait testing
struct Point {
    int32_t x{0};
    int32_t y{0};
    constexpr Point(int32_t x_ = 0, int32_t y_ = 0) noexcept : x(x_), y(y_) {}
    bool operator==(const Point &o) const noexcept { return x == o.x && y == o.y; }
};

// Custom trait specialization
template <>
struct argparse::value_parser<Point> {
    static argparse::expected<Point, argparse::parse_error> parse(argparse::string_view sv) {
        auto comma = sv.find(',');
        if (comma == argparse::string_view::npos) {
            return argparse::unexpected<argparse::parse_error>(
                argparse::parse_error{argparse::error_code::invalid_value, sv, "Point requires 'x,y' format"});
        }
        auto x_part = sv.substr(0, comma);
        auto y_part = sv.substr(comma + 1);

        auto x_res = argparse::value_parser<int32_t>::parse(x_part);
        if (!x_res)
            return argparse::unexpected<argparse::parse_error>(x_res.error());

        auto y_res = argparse::value_parser<int32_t>::parse(y_part);
        if (!y_res)
            return argparse::unexpected<argparse::parse_error>(y_res.error());

        return Point{*x_res, *y_res};
    }
};

TEST_CASE(test_fixed_width_integers) {
    argparse::argument_parser parser("test");
    parser.add_argument("--i32").default_value<int32_t>(0);
    parser.add_argument("--u32").default_value<uint32_t>(0);
    parser.add_argument("--i64").default_value<int64_t>(0);
    parser.add_argument("--u64").default_value<uint64_t>(0);
    parser.add_argument("--hex").default_value<uint32_t>(0);
    parser.add_argument("--bin").default_value<uint32_t>(0);

    std::vector<argparse::string_view> args = {"--i32", "-12345",      "--u32", "54321", "--i64", "-9000000000",
                                               "--u64", "18000000000", "--hex", "0x2A",  "--bin", "0b1010"};

    auto res = parser.parse_args(args);
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ((*res).get<int32_t>("--i32"), -12345);
    ASSERT_EQ((*res).get<uint32_t>("--u32"), 54321u);
    ASSERT_EQ((*res).get<int64_t>("--i64"), -9000000000LL);
    ASSERT_EQ((*res).get<uint64_t>("--u64"), 18000000000ULL);
    ASSERT_EQ((*res).get<uint32_t>("--hex"), 42u);
    ASSERT_EQ((*res).get<uint32_t>("--bin"), 10u);
}

TEST_CASE(test_floating_point) {
    argparse::argument_parser parser("test");
    parser.add_argument("--flt");
    parser.add_argument("--dbl");

    std::vector<argparse::string_view> args = {"--flt", "3.14", "--dbl", "2.71828"};

    auto res = parser.parse_args(args);
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(std::abs((*res).get<float>("--flt") - 3.14f) < 0.001f);
    ASSERT_TRUE(std::abs((*res).get<double>("--dbl") - 2.71828) < 0.00001);
}

TEST_CASE(test_boolean_variants) {
    argparse::argument_parser parser("test");
    parser.add_argument("--b1");
    parser.add_argument("--b2");
    parser.add_argument("--b3");
    parser.add_argument("--b4");

    std::vector<argparse::string_view> args = {"--b1", "true", "--b2", "yes", "--b3", "0", "--b4", "off"};

    auto res = parser.parse_args(args);
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ((*res).get<bool>("--b1"), true);
    ASSERT_EQ((*res).get<bool>("--b2"), true);
    ASSERT_EQ((*res).get<bool>("--b3"), false);
    ASSERT_EQ((*res).get<bool>("--b4"), false);
}

TEST_CASE(test_vector_and_containers) {
    argparse::argument_parser parser("test");
    parser.add_argument("--nums");
    parser.add_argument("--tags").append();

    std::vector<argparse::string_view> args = {"--nums", "10,20,30,40", "--tags", "tag1",
                                               "--tags", "tag2",        "--tags", "tag3"};

    auto res = parser.parse_args(args);
    ASSERT_TRUE(res.has_value());

    auto nums = (*res).get<std::vector<int32_t>>("--nums");
    ASSERT_EQ(nums.size(), 4u);
    ASSERT_EQ(nums[0], 10);
    ASSERT_EQ(nums[1], 20);
    ASSERT_EQ(nums[2], 30);
    ASSERT_EQ(nums[3], 40);

    auto tags = (*res).get<std::vector<std::string>>("--tags");
    ASSERT_EQ(tags.size(), 3u);
    ASSERT_EQ(tags[0], "tag1");
    ASSERT_EQ(tags[1], "tag2");
    ASSERT_EQ(tags[2], "tag3");
}

TEST_CASE(test_custom_trait_specialization) {
    argparse::argument_parser parser("test");
    parser.add_argument("--point");

    std::vector<argparse::string_view> args = {"--point", "128,256"};

    auto res = parser.parse_args(args);
    ASSERT_TRUE(res.has_value());

    auto pt = (*res).get<Point>("--point");
    ASSERT_EQ(pt.x, 128);
    ASSERT_EQ(pt.y, 256);
}

int main() {
    return test_framework::run_all();
}
