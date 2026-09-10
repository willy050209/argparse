# argparse: Multi-Standard High-Performance C++ Command-Line Parser (C++11/17/20/23)

[![CI](https://github.com/modern-cpp/argparse/actions/workflows/ci.yml/badge.svg)](https://github.com/modern-cpp/argparse/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Standard](https://img.shields.io/badge/C%2B%2B-11%20%7C%2017%20%7C%2020%20%7C%2023-blue.svg)](https://en.cppreference.com/w/cpp/23)

A type-safe, zero-overhead, pure-functional command-line argument parser designed for modern C++ (prioritizing C++23 while seamlessly backward-compatible across C++11, C++17, and C++20).

---

## ✨ Features

- **Multi-Standard Support (C++11 / 17 / 20 / 23)**:
  - **C++23 First**: Native `std::expected<T, E>`, monadic chaining (`.and_then()`, `.transform()`), `std::print` / `std::println`, compile-time concepts, and standard library modules.
  - **Backward Compatibility**: Fully functional zero-dependency polyfills and SFINAE fallbacks when compiling with C++11, C++17, or C++20.
- **Modern `std::print` Priority**:
  - Automatically prioritizes `std::print` / `std::println` via `argparse::print` and `argparse::println`.
  - Zero-overhead fallback to `std::format` or buffered streaming on older standards.
- **C++20 Module Export**:
  - Provides `include/argparse/argparse.cppm` with `export module argparse;` for next-generation module build systems.
- **CLI Schema Model Export**:
  - Direct export of parser schema via `parser.export_model()` (`cli_model` structure) and `parser.to_json()` for tooling, shell auto-completion generation, and automated documentation.
- **Flexible Argument Specification**:
  - Positional arguments, short options (`-v`), long options (`--verbose`).
  - Short flag chaining (e.g., `-xvf` expands to `-x`, `-v`, `-f`).
  - Inline and space assignment (`--output=file.txt` and `--output file.txt`).
  - Option delimiter support (`--` ends options processing).
- **Rich Constraints & Ergonomics**:
  - Required and optional arguments.
  - Default values and implicit flag values.
  - Allowed choices validation (`.choices(...)`).
  - Mutually exclusive groups (`add_mutually_exclusive_group()`).
  - Custom validation predicates with descriptive failure messages.
- **Pure-Functional Core**:
  - No global state or parser side-effects during parsing.
  - Produces immutable `parse_result` objects.
- **Zero Dependencies**:
  - Header-only library; can be consumed as modular headers, C++20 module, or a single amalgamated header.

---

## 🚀 Quick Start

### Basic Usage

```cpp
#include <argparse/argparse.hpp>

int main(int argc, char* argv[]) {
    argparse::argument_parser program("network_tool", "High-performance packet dispatcher");

    program.add_argument("--port", "-p")
        .help("Port number to listen on")
        .default_value<int32_t>(8080)
        .validator<int32_t>([](int32_t p) { return p > 0 && p <= 65535; }, "Port must be in range 1-65535");

    program.add_argument("--mode", "-m")
        .help("Execution mode")
        .choices("tcp", "udp", "raw")
        .default_value(std::string("tcp"));

    auto& group = program.add_mutually_exclusive_group();
    group.add_argument("--daemon", "-d").help("Run in daemon mode").flag();
    group.add_argument("--interactive", "-i").help("Run interactively").flag();

    auto result = program.parse_args(argc, argv);
    if (!result) {
        argparse::println(std::cerr, "Parse error: {}", result.error().to_string());
        argparse::print(std::cerr, "{}", program.format_help());
        return 1;
    }

    auto port = result->get<int32_t>("--port");
    auto mode = result->get<std::string>("--mode");
    bool is_daemon = result->has("--daemon");

    argparse::println("Listening on port {} in mode [{}] (daemon: {})", port, mode, is_daemon);
    return 0;
}
```

### CLI Schema Export (JSON & Model)

```cpp
argparse::cli_model model = program.export_model();
std::string json_schema = program.to_json();
argparse::println("Schema:\n{}", json_schema);
```

---

## 📦 Integration

### CMake FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/modern-cpp/argparse.git
    GIT_TAG main
)
FetchContent_MakeAvailable(argparse)

target_link_libraries(my_target PRIVATE argparse::argparse)
```

### Single Header Amalgamation

Download `single_include/argparse/argparse.hpp` from the [Releases](https://github.com/modern-cpp/argparse/releases) page and include it directly in your project.

---

## 🛠️ Building & Testing

Compatible with CMake 3.28+ and standard C++ compilers:
- **Windows**: Visual Studio 18 Insiders MSVC (14.51+) / Visual Studio 2022
- **Linux (WSL/Ubuntu)**: GCC 11+ / GCC 14+ / Clang 14+

```bash
# Build & run tests
cmake -B build -DCMAKE_BUILD_TYPE=Release -DARGPARSE_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

---

## 📄 License

Distributed under the MIT License. See [LICENSE](LICENSE) for details.
