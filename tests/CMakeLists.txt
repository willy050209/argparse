cmake_minimum_required(VERSION 3.28)

function(add_argparse_test test_name)
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE argparse argparse_warnings)
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

add_argparse_test(test_primitive_parsing)
add_argparse_test(test_flag_chaining)
add_argparse_test(test_constraints)
add_argparse_test(test_compat_detect)
add_argparse_test(test_error_handling)
add_argparse_test(test_model_export)

# Single-header test target
add_executable(test_single_header test_single_header.cpp)
target_include_directories(test_single_header PRIVATE ${PROJECT_SOURCE_DIR}/single_include)
target_link_libraries(test_single_header PRIVATE argparse_warnings)
add_test(NAME test_single_header COMMAND test_single_header)
