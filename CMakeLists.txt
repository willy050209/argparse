cmake_minimum_required(VERSION 3.28)

project(
    argparse
    VERSION 1.0.0
    DESCRIPTION "High-performance, type-safe, multi-standard (C++11/17/20/23) command-line argument parser"
    HOMEPAGE_URL "https://github.com/modern-cpp/argparse"
    LANGUAGES CXX
)

# Standard C++ configuration (Default 23, but supports 11, 17, 20)
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 23)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Project options
option(ARGPARSE_BUILD_TESTS "Build test suite" ON)
option(ARGPARSE_BUILD_EXAMPLES "Build example executables" ON)
option(ARGPARSE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

# Define header-only interface library
add_library(argparse INTERFACE)
add_library(argparse::argparse ALIAS argparse)

target_include_directories(argparse INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Compiler warnings interface
add_library(argparse_warnings INTERFACE)
if(MSVC)
    target_compile_options(argparse_warnings INTERFACE
        /W4
        /permissive-
        /utf-8
        /Zc:__cplusplus
        /Zc:preprocessor
        $<$<BOOL:${ARGPARSE_WARNINGS_AS_ERRORS}>:/WX>
    )
else()
    target_compile_options(argparse_warnings INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        $<$<BOOL:${ARGPARSE_WARNINGS_AS_ERRORS}>:-Werror>
    )
endif()

# Installation rules
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(
    TARGETS argparse
    EXPORT argparseTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT argparseTargets
    FILE argparseTargets.cmake
    NAMESPACE argparse::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/argparse
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/argparseConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/argparseConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/argparse
)

# Testing
if(ARGPARSE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples
if(ARGPARSE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
