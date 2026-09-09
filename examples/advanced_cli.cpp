#include <argparse/argparse.hpp>
#include <iostream>

int main(int argc, char *argv[]) {
    argparse::argument_parser server("micro_server", "High-performance Network Daemon");

    server.add_argument("--port", "-p")
        .help("Listening port number (1024-65535)")
        .default_value<int32_t>(8080)
        .validator<int32_t>([](int32_t port) { return port >= 1024 && port <= 65535; },
                            "Port must be in unprivileged range 1024-65535");

    server.add_argument("--protocol")
        .help("Communication protocol")
        .choices("tcp", "udp", "quic")
        .default_value(std::string("tcp"));

    server.add_argument("--routes", "-r").help("Initial endpoints list").append();

    auto &group = server.add_mutually_exclusive_group();
    group.add_argument("--daemon", "-d").help("Fork into background").flag();
    group.add_argument("--foreground", "-f").help("Stay in foreground").flag();

    auto result = server.parse_args(argc, argv);
    if (!result) {
        argparse::println(std::cerr, "Configuration error: {}", result.error().to_string());
        argparse::println(std::cerr);
        argparse::print(std::cerr, "{}", server.format_help());
        return 1;
    }

    if (result->has("--help") || result->has("-h")) {
        argparse::print("{}", server.format_help());
        return 0;
    }

    auto port = result->get<int32_t>("--port");
    auto protocol = result->get<std::string>("--protocol");
    auto routes = result->try_get<std::vector<std::string>>("--routes").value_or(std::vector<std::string>{});
    bool is_daemon = result->get_or<bool>("--daemon", false);

    argparse::println("Starting server on port {} ({})", port, protocol);
    argparse::println("Mode: {}", (is_daemon ? "Daemon" : "Foreground"));
    argparse::println("Configured routes count: {}", routes.size());
    for (const auto &route : routes) {
        argparse::println("  - {}", route);
    }

    return 0;
}
