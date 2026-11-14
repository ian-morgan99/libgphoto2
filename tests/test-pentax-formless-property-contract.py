#!/usr/bin/env python3
"""Source contract for form-less Pentax property domains (issue #164)."""

import sys


def function_body(source: str, signature: str) -> str:
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start:index + 1]
    raise AssertionError("unterminated function")


def strip_comments(source: str) -> str:
    out = []
    index = 0
    while index < len(source):
        if source.startswith("/*", index):
            end = source.find("*/", index + 2)
            index = len(source) if end < 0 else end + 2
        elif source.startswith("//", index):
            end = source.find("\n", index)
            index = len(source) if end < 0 else end
        else:
            out.append(source[index])
            index += 1
    return "".join(out)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: test-pentax-formless-property-contract.py <config.c>",
              file=sys.stderr)
        return 2
    config = strip_comments(open(sys.argv[1], encoding="utf-8").read())
    getter = function_body(config, "_pentax_prop_get (Camera *camera,")
    setter = function_body(config, "_pentax_prop_put (Camera *camera,")

    require("code != PTP_DPC_PENTAX_ColorTemperature" in getter,
            "unknown form-less properties must fail closed in the getter")
    require("desc.DataType != PTP_DTC_UINT16" in getter,
            "color temperature must remain a UINT16 source contract")
    domain = setter.index("pentax_color_temperature_value_is_source_valid")
    write = setter.index("ptp_setdevicepropvalue")
    require(domain < write,
            "form-less color temperature domain validation must precede PTP write")
    require("GP_ERROR_BAD_PARAMETERS" in setter,
            "out-of-domain color temperature must return bad parameters")
    require("GP_ERROR_NOT_SUPPORTED" in setter,
            "unknown form-less scalar must remain unsupported")

    print("test-pentax-formless-property-contract: all checks passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, OSError, ValueError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
