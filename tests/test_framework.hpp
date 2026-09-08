#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
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

inline bool register_test(const std::string& name, std::function<void()> func) {
    registry().push_back(test_case{name, std::move(func)});
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

#if defined(__GNUC__) || defined(__clang__)
    #define ARGPARSE_UNUSED_TEST __attribute__((unused))
#else
    #define ARGPARSE_UNUSED_TEST
#endif

#define TEST_CASE(name) \
    static void name() ARGPARSE_UNUSED_TEST; \
    namespace { \
        const bool name##_registered ARGPARSE_UNUSED_TEST = ::test_framework::register_test(#name, name); \
    } \
    static void name()

#define ASSERT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            std::ostringstream oss; \
            oss << "Assertion failed: (" << #expr << ") at " << __FILE__ << ":" << __LINE__; \
            throw ::test_framework::assertion_failure(oss.str()); \
        } \
    } while (false)

#define ASSERT_FALSE(expr) \
    do { \
        if (expr) { \
            std::ostringstream oss; \
            oss << "Assertion failed: !(" << #expr << ") at " << __FILE__ << ":" << __LINE__; \
            throw ::test_framework::assertion_failure(oss.str()); \
        } \
    } while (false)

#define ASSERT_EQ(lhs, rhs) \
    do { \
        if (!((lhs) == (rhs))) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " << #lhs << " == " << #rhs << " at " << __FILE__ << ":" << __LINE__; \
            throw ::test_framework::assertion_failure(oss.str()); \
        } \
    } while (false)

#define ASSERT_NE(lhs, rhs) \
    do { \
        if ((lhs) == (rhs)) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " << #lhs << " != " << #rhs << " at " << __FILE__ << ":" << __LINE__; \
            throw ::test_framework::assertion_failure(oss.str()); \
        } \
    } while (false)
