#pragma once

/**
 * @file argparse.hpp
 * @brief High-performance, zero-overhead, multi-standard (C++11/17/20/23) command-line parser.
 */

#include <argparse/compat/detect.hpp>
#include <argparse/compat/expected.hpp>
#include <argparse/compat/print.hpp>
#include <argparse/compat/span.hpp>
#include <argparse/compat/string_view.hpp>
#include <argparse/compat/traits.hpp>
#include <argparse/config/action.hpp>
#include <argparse/config/argument.hpp>
#include <argparse/config/argument_group.hpp>
#include <argparse/config/parser.hpp>
#include <argparse/core/error.hpp>
#include <argparse/core/token.hpp>
#include <argparse/core/value_parser.hpp>
#include <argparse/engine/engine.hpp>
#include <argparse/engine/parse_result.hpp>
#include <argparse/engine/tokenizer.hpp>
#include <argparse/format/formatter.hpp>
#include <argparse/model/cli_model.hpp>
