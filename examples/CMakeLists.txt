cmake_minimum_required(VERSION 3.28)

add_executable(basic_cli basic_cli.cpp)
target_link_libraries(basic_cli PRIVATE argparse argparse_warnings)

add_executable(advanced_cli advanced_cli.cpp)
target_link_libraries(advanced_cli PRIVATE argparse argparse_warnings)
