#include <argparse/argparse.hpp>
#include <iostream>

int main(int argc, char* argv[]) {
    argparse::argument_parser server("micro_server", "High-performance C++23 Network Daemon");

    server.add_argument("--port", "-p")
        .help("Listening port number (1024-65535)")
        .default_value<int32_t>(8080)
        .validator<int32_t>([](int32_t port) {
            return port >= 1024 && port <= 65535;
        }, "Port must be in unprivileged range 1024-65535");

    server.add_argument("--protocol")
        .help("Communication protocol")
        .choices("tcp", "udp", "quic")
        .default_value(std::string("tcp"));

    server.add_argument("--routes", "-r")
        .help("Initial endpoints list")
        .append();

    auto& group = server.add_mutually_exclusive_group();
    group.add_argument("--daemon", "-d").help("Fork into background").flag();
    group.add_argument("--foreground", "-f").help("Stay in foreground").flag();

    auto result = server.parse_args(argc, argv);
    if (!result) {
        std::cerr << "Configuration error: " << result.error().to_string() << "\n\n";
        std::cerr << server.format_help() << "\n";
        return 1;
    }

    if (result->has("--help")) {
        std::cout << server.format_help();
        return 0;
    }

    auto port = result->get<int32_t>("--port");
    auto protocol = result->get<std::string>("--protocol");
    auto routes = result->try_get<std::vector<std::string>>("--routes").value_or(std::vector<std::string>{});
    bool is_daemon = result->get_or<bool>("--daemon", false);

    std::cout << "Starting server on port " << port << " (" << protocol << ")\n";
    std::cout << "Mode: " << (is_daemon ? "Daemon" : "Foreground") << "\n";
    std::cout << "Configured routes count: " << routes.size() << "\n";
    for (const auto& route : routes) {
        std::cout << "  - " << route << "\n";
    }

    return 0;
}
