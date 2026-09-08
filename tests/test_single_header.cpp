#include <argparse/argparse.hpp>
#include <cassert>
#include <iostream>
#include <vector>

int main() {
    argparse::argument_parser parser("single_header_test", "Verifies single-header self-containment");
    parser.add_argument("--message", "-m").default_value(std::string("hello single header"));
    parser.add_argument("--count", "-c").default_value<int32_t>(42);
    parser.add_argument("-v").flag();

    std::vector<std::string_view> args = { "-m", "world", "-v", "-c", "99" };
    auto res = parser.parse_args(args);

    assert(res.has_value());
    assert(res->get<std::string>("-m") == "world");
    assert(res->get<bool>("-v") == true);
    assert(res->get<int32_t>("-c") == 99);

    std::cout << "All single-header self-containment checks passed!\n";
    return 0;
}
