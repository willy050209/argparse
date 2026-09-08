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
    "core/error.hpp",
    "core/traits.hpp",
    "core/value_parser.hpp",
    "core/token.hpp",
    "config/action.hpp",
    "config/argument.hpp",
    "config/argument_group.hpp",
    "engine/parse_result.hpp",
    "engine/tokenizer.hpp",
    "engine/engine.hpp",
    "format/formatter.hpp",
    "config/parser.hpp",
]

BANNER = """// ============================================================================
// argparse: High-performance, type-safe, zero-overhead C++23 argument parser
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
                m = std_include_re.match(line)
                if m:
                    std_includes.add(m.group(1))
                    continue
                cleaned_lines.append(line)

        content = "".join(cleaned_lines).strip()
        file_contents.append(f"// --- Begin: {rel_path} ---\n{content}\n// --- End: {rel_path} ---\n")

    sorted_std_includes = sorted(list(std_includes))
    include_block = "\n".join(f"#include <{inc}>" for inc in sorted_std_includes)

    final_content = BANNER + include_block + "\n\n" + "\n\n".join(file_contents) + "\n"

    with open(output_file, "w", encoding="utf-8") as f:
        f.write(final_content)

    print(f"Successfully generated single-header: {output_file} ({len(final_content)} bytes)")

if __name__ == "__main__":
    amalgamate()
