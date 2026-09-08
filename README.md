# argparse: Modern C++23 High-Performance Command-Line Parser

[![CI](https://github.com/modern-cpp/argparse/actions/workflows/ci.yml/badge.svg)](https://github.com/modern-cpp/argparse/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Standard](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)

A type-safe, zero-overhead, pure-functional command-line argument parser for modern C++ (C++23).

---

## ✨ Features

- **C++23 First-Class Design**:
  - `std::expected<T, E>` based error handling with monadic chaining (`.and_then()`, `.transform()`).
  - Zero-string-copy parsing using `std::string_view` and `std::span`.
  - Non-allocating fast primitive conversion with `std::from_chars` (locale-independent).
  - Explicit fixed-width integer types (`int32_t`, `int64_t`, `uint32_t`, `uint64_t`) from `<cstdint>`.
  - Compile-time concepts (`argparse::parsable<T>`, `argparse::validator_for<F, T>`).
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
  - Header-only library; can be consumed as modular headers or a single amalgamated header.

---

## 🚀 Quick Start

### Basic Usage

```cpp
#include <argparse/argparse.hpp>
#include <iostream>

int main(int argc, char* argv[]) {
    argparse::argument_parser program("network_tool", "High-performance packet dispatcher");

    program.add_argument("--port", "-p")
        .help("Port number to listen on")
        .default_value<int32_t>(8080)
        .validator([](int32_t p) { return p > 0 && p <= 65535; }, "Port must be in range 1-65535");

    program.add_argument("--mode", "-m")
        .help("Execution mode")
        .choices("tcp", "udp", "raw")
        .default_value(std::string("tcp"));

    auto& group = program.add_mutually_exclusive_group();
    group.add_argument("--daemon", "-d").help("Run in daemon mode").flag();
    group.add_argument("--interactive", "-i").help("Run interactively").flag();

    auto result = program.parse_args(argc, argv);
    if (!result) {
        std::cerr << "Parse error: " << result.error().to_string() << "\n";
        std::cerr << program.format_help() << "\n";
        return 1;
    }

    auto port = result->get<int32_t>("--port");
    auto mode = result->get<std::string>("--mode");
    bool is_daemon = result->has("--daemon");

    std::cout << "Listening on port " << port << " in mode [" << mode << "]\n";
    return 0;
}
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

Requires CMake 3.28+ and a C++23 compliant compiler (GCC 13+, Clang 17+, or MSVC 2022 v143+).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DARGPARSE_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

---

## 📄 License

Distributed under the MIT License. See [LICENSE](LICENSE) for details.
