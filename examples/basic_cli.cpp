#include <argparse/argparse.hpp>

int main(int argc, char *argv[]) {
    argparse::argument_parser program("basic_cli", "A modern C++ example CLI application");

    program.add_argument("--name", "-n").help("Your name").default_value(std::string("Guest"));

    program.add_argument("--count", "-c").help("Number of greetings to print").default_value<int32_t>(1);

    program.add_argument("--verbose", "-v").help("Enable verbose logging").flag();

    program.add_argument("target").help("Target destination").default_value(std::string("World"));

    auto result = program.parse_or_exit(argc, argv);

    auto name = result.get<std::string>("--name");
    auto count = result.get<int32_t>("--count");
    auto target = result.get<std::string>("target");
    bool verbose = result.get<bool>("--verbose");

    if (verbose) {
        argparse::println("[VERBOSE] Preparing greeting for {} targeting {}", name, target);
    }

    for (int32_t i = 0; i < count; ++i) {
        argparse::println("Hello, {}! Welcome to {}!", name, target);
    }

    return 0;
}
