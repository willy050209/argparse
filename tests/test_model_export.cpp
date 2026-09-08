#include "test_framework.hpp"
#include <argparse/argparse.hpp>

TEST_CASE(test_export_model) {
    argparse::argument_parser parser("tool", "CLI tool with model export");
    parser.version("2.1.0");

    parser.add_argument("--port", "-p")
        .help("Port number")
        .default_value<int32_t>(8080);

    parser.add_argument("--protocol")
        .help("Network protocol")
        .choices("tcp", "udp");

    auto& grp = parser.add_mutually_exclusive_group();
    grp.add_argument("--quiet", "-q").flag();
    grp.add_argument("--verbose", "-v").flag();

    argparse::cli_model model = parser.export_model();

    ASSERT_EQ(model.program_name, "tool");
    ASSERT_EQ(model.description, "CLI tool with model export");
    ASSERT_EQ(model.version, "2.1.0");
    ASSERT_EQ(model.mutually_exclusive_groups.size(), 1u);

    std::string json = parser.to_json();
    ASSERT_TRUE(json.find("\"program_name\": \"tool\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"version\": \"2.1.0\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"choices\": [\"tcp\", \"udp\"]") != std::string::npos);
}

int main() {
    return test_framework::run_all();
}
