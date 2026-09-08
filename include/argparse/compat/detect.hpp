#pragma once

#if defined(_MSVC_LANG)
    #define ARGPARSE_CPLUSPLUS _MSVC_LANG
#else
    #define ARGPARSE_CPLUSPLUS __cplusplus
#endif

#if defined(__has_include)
    #if __has_include(<version>)
        #include <version>
    #endif
#endif

#if ARGPARSE_CPLUSPLUS >= 201402L
    #define ARGPARSE_CONSTEXPR14 constexpr
#else
    #define ARGPARSE_CONSTEXPR14
#endif

// Detect std::print / std::println (C++23)
#if defined(__cpp_lib_print) && __cpp_lib_print >= 202207L
    #define ARGPARSE_HAS_STD_PRINT 1
#elif defined(__has_include)
    #if __has_include(<print>) && ARGPARSE_CPLUSPLUS >= 202302L
        #define ARGPARSE_HAS_STD_PRINT 1
    #else
        #define ARGPARSE_HAS_STD_PRINT 0
    #endif
#else
    #define ARGPARSE_HAS_STD_PRINT 0
#endif

// Detect std::expected (C++23)
#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
    #define ARGPARSE_HAS_STD_EXPECTED 1
#elif defined(__has_include)
    #if __has_include(<expected>) && ARGPARSE_CPLUSPLUS >= 202302L
        #define ARGPARSE_HAS_STD_EXPECTED 1
    #else
        #define ARGPARSE_HAS_STD_EXPECTED 0
    #endif
#else
    #define ARGPARSE_HAS_STD_EXPECTED 0
#endif

// Detect std::span (C++20)
#if defined(__cpp_lib_span) && __cpp_lib_span >= 202002L
    #define ARGPARSE_HAS_STD_SPAN 1
#elif defined(__has_include)
    #if __has_include(<span>) && ARGPARSE_CPLUSPLUS >= 202002L
        #define ARGPARSE_HAS_STD_SPAN 1
    #else
        #define ARGPARSE_HAS_STD_SPAN 0
    #endif
#else
    #define ARGPARSE_HAS_STD_SPAN 0
#endif

// Detect std::string_view (C++17)
#if defined(__cpp_lib_string_view) && __cpp_lib_string_view >= 201606L
    #define ARGPARSE_HAS_STD_STRING_VIEW 1
#elif defined(__has_include)
    #if __has_include(<string_view>) && ARGPARSE_CPLUSPLUS >= 201703L
        #define ARGPARSE_HAS_STD_STRING_VIEW 1
    #else
        #define ARGPARSE_HAS_STD_STRING_VIEW 0
    #endif
#else
    #define ARGPARSE_HAS_STD_STRING_VIEW 0
#endif

// Detect Concepts (C++20)
#if defined(__cpp_concepts) && __cpp_concepts >= 201907L
    #define ARGPARSE_HAS_CONCEPTS 1
#else
    #define ARGPARSE_HAS_CONCEPTS 0
#endif

// Detect std::format (C++20)
#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
    #define ARGPARSE_HAS_STD_FORMAT 1
#elif defined(__has_include)
    #if __has_include(<format>) && ARGPARSE_CPLUSPLUS >= 202002L
        #define ARGPARSE_HAS_STD_FORMAT 1
    #else
        #define ARGPARSE_HAS_STD_FORMAT 0
    #endif
#else
    #define ARGPARSE_HAS_STD_FORMAT 0
#endif
