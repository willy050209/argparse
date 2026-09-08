#pragma once

/**
 * @file argparse.hpp
 * @brief High-performance, zero-overhead, type-safe C++23 command-line parser.
 */

#include <argparse/config/action.hpp>
#include <argparse/config/argument.hpp>
#include <argparse/config/argument_group.hpp>
#include <argparse/config/parser.hpp>
#include <argparse/core/error.hpp>
#include <argparse/core/token.hpp>
#include <argparse/core/traits.hpp>
#include <argparse/core/value_parser.hpp>
#include <argparse/engine/engine.hpp>
#include <argparse/engine/parse_result.hpp>
#include <argparse/engine/tokenizer.hpp>
#include <argparse/format/formatter.hpp>
