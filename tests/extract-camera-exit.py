#!/usr/bin/env python3
"""Extract unchanged production functions for the hardware-free cleanup test.

This compiles the real control flow against real camlib types and mocked
transport helpers, without making a test-only symbol part of the camlib ABI.
"""

import pathlib
import re
import sys


def extract(source, signature):
    start = source.index(signature)
    opening = source.index("{", start)
    # Ignore braces inside comments and string/character literals.
    tokens = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|'
                        r"'(?:\\.|[^'\\])*'|[{}]", re.S)
    depth = 0
    for match in tokens.finditer(source, opening):
        token = match.group()
        if token == "{":
            depth += 1
        elif token == "}":
            depth -= 1
            if depth == 0:
                return source[start:match.end()]
    raise ValueError("unterminated function: " + signature)


if __name__ == "__main__":
    source = pathlib.Path(sys.argv[1]).read_text(encoding="utf-8")
    functions = [extract(source, signature) for signature in (
        "int\ntranslate_ptp_result (uint16_t result)",
        "static int\ncamera_exit (Camera *camera, GPContext *context)",
    )]
    pathlib.Path(sys.argv[2]).write_text(
        "/* Generated from library.c; do not edit. */\n" +
        "\n\n".join(functions) + "\n", encoding="utf-8")
