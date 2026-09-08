#!/usr/bin/env python3
"""
Single-Header Amalgamator for argparse.
Combines all modular headers in include/argparse/ into a single self-contained header:
single_include/argparse/argparse.hpp
"""

import os
import re
from pathlib import Path

HEADER_ORDER = [
    "compat/detect.hpp",
    "compat/string_view.hpp",
    "compat/span.hpp",
    "compat/expected.hpp",
    "compat/traits.hpp",
    "compat/print.hpp",
    "core/error.hpp",
    "core/traits.hpp",
    "core/token.hpp",
    "core/value_parser.hpp",
    "config/action.hpp",
    "config/argument.hpp",
    "config/argument_group.hpp",
    "engine/parse_result.hpp",
    "engine/tokenizer.hpp",
    "engine/engine.hpp",
    "format/formatter.hpp",
    "model/cli_model.hpp",
    "config/parser.hpp",
]

BANNER = """// ============================================================================
// argparse: High-performance, multi-standard (C++11/17/20/23) argument parser
// https://github.com/modern-cpp/argparse
// Distributed under the MIT License.
// ============================================================================
#pragma once

"""

def amalgamate():
    repo_root = Path(__file__).resolve().parent.parent
    include_dir = repo_root / "include" / "argparse"
    output_dir = repo_root / "single_include" / "argparse"
    output_dir.mkdir(parents=True, exist_ok=True)
    output_file = output_dir / "argparse.hpp"

    std_includes = set()
    file_contents = []

    std_include_re = re.compile(r'^\s*#include\s*<([^>]+)>\s*$')
    argparse_include_re = re.compile(r'^\s*#include\s*<argparse/[^>]+>\s*$')
    pragma_once_re = re.compile(r'^\s*#pragma\s+once\s*$')

    for rel_path in HEADER_ORDER:
        full_path = include_dir / rel_path
        if not full_path.exists():
            raise FileNotFoundError(f"Header not found: {full_path}")

        cleaned_lines = []
        with open(full_path, "r", encoding="utf-8") as f:
            for line in f:
                if pragma_once_re.match(line):
                    continue
                if argparse_include_re.match(line):
                    continue
                # Keep conditional/guarded includes as is, only extract top level unconditional std headers
                cleaned_lines.append(line)

        content = "".join(cleaned_lines).strip()
        file_contents.append(f"// --- Begin: {rel_path} ---\n{content}\n// --- End: {rel_path} ---\n")

    final_content = BANNER + "\n\n" + "\n\n".join(file_contents) + "\n"

    with open(output_file, "w", encoding="utf-8") as f:
        f.write(final_content)

    print(f"Successfully generated single-header: {output_file} ({len(final_content)} bytes)")

if __name__ == "__main__":
    amalgamate()
