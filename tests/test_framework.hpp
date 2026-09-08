#pragma once

#include <exception>
#include <format>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace test_framework {

struct test_case {
    std::string name;
    std::function<void()> func;
};

inline std::vector<test_case>& registry() {
    static std::vector<test_case> tests;
    return tests;
}

inline bool register_test(std::string_view name, std::function<void()> func) {
    registry().push_back(test_case{std::string(name), std::move(func)});
    return true;
}

inline int run_all() {
    int passed = 0;
    int failed = 0;

    std::cout << "[==========] Running " << registry().size() << " test(s).\n";

    for (const auto& tc : registry()) {
        std::cout << "[ RUN      ] " << tc.name << "\n";
        try {
            tc.func();
            std::cout << "[       OK ] " << tc.name << "\n";
            passed++;
        } catch (const std::exception& ex) {
            std::cerr << "[  FAILED  ] " << tc.name << ": " << ex.what() << "\n";
            failed++;
        } catch (...) {
            std::cerr << "[  FAILED  ] " << tc.name << ": Unknown exception\n";
            failed++;
        }
    }

    std::cout << "[==========] " << registry().size() << " test(s) ran.\n";
    std::cout << "[  PASSED  ] " << passed << " test(s).\n";
    if (failed > 0) {
        std::cerr << "[  FAILED  ] " << failed << " test(s).\n";
        return 1;
    }
    return 0;
}

struct assertion_failure : public std::exception {
    std::string msg;
    explicit assertion_failure(std::string m) : msg(std::move(m)) {}
    [[nodiscard]] const char* what() const noexcept override {
        return msg.c_str();
    }
};

} // namespace test_framework

#define TEST_CASE(name) \
    static void name(); \
    static const bool name##_registered = ::test_framework::register_test(#name, name); \
    static void name()

#define ASSERT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            throw ::test_framework::assertion_failure( \
                std::format("Assertion failed: ({}) at {}:{}", #expr, __FILE__, __LINE__)); \
        } \
    } while (false)

#define ASSERT_FALSE(expr) \
    do { \
        if (expr) { \
            throw ::test_framework::assertion_failure( \
                std::format("Assertion failed: !({}) at {}:{}", #expr, __FILE__, __LINE__)); \
        } \
    } while (false)

#define ASSERT_EQ(lhs, rhs) \
    do { \
        if (!((lhs) == (rhs))) { \
            throw ::test_framework::assertion_failure( \
                std::format("Assertion failed: {} == {} at {}:{}", #lhs, #rhs, __FILE__, __LINE__)); \
        } \
    } while (false)

#define ASSERT_NE(lhs, rhs) \
    do { \
        if ((lhs) == (rhs)) { \
            throw ::test_framework::assertion_failure( \
                std::format("Assertion failed: {} != {} at {}:{}", #lhs, #rhs, __FILE__, __LINE__)); \
        } \
    } while (false)
