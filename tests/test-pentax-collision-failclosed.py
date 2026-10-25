#!/usr/bin/env python3
"""Keep Pentax collision probes fail-closed (PR #110 review requirement).

This is a source-contract test, not a hardware qualification test. It exists because the
stateful integration harness the review asked for -- injecting a filesystem result into
`camera_pentax_capture_internal` and counting transfer/delete calls -- cannot be built
offline: `library.c` is only ever consumed by `test-pentax-capture-contract.py` as text,
never linked into a C unit test (see `tests/meson.build`). So the invariants that harness
would assert are pinned here at source level, and the stateful counters remain an explicit
outstanding item rather than a claimed pass.

What is pinned, and why each one matters:

1. The predicate accepts ONLY GP_ERROR_FILE_NOT_FOUND. gp_filesystem_number() returns
   FILE_NOT_FOUND for "clean folder, name absent", DIRECTORY_NOT_FOUND for a missing folder,
   or a positive index. GP_ERROR_BAD_PARAMETERS can only come from C_PARAMS(fs && folder &&
   filename) -- NULL arguments, i.e. a caller bug -- so treating it as "slot is free" could
   publish a companion over a real file.

2. Every collision-probe call site guards with that predicate and returns the probe's own
   error, preserving the camera candidate instead of publishing over it. A call site that
   silently ignored a negative result would reintroduce the defect the PR fixes.

3. The suffix formatter is called through pentax_collision_suffix_name(), never by writing
   into the same buffer strrchr() was reading from. The pre-PR code did
   `dot = strrchr(path->name,'.')` then `snprintf(path->name, ..., "%s_%d%s", stem, suffix, dot)`,
   aliasing source and destination -- undefined behaviour.

4. Both the primary and companion retry paths use the shared helper, so a future edit cannot
   fix one and leave the other aliased.

Run: python3 tests/test-pentax-collision-failclosed.py <path-to-library.c>
Exit 0 = contract holds, 1 = violated, 77 = prerequisite missing (source file absent).
"""

import sys


def function_body(source: str, signature: str) -> str:
    if signature not in source:
        raise AssertionError(f"symbol not found in source: {signature}")
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
    raise AssertionError(f"unterminated function: {signature}")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def check_predicate(utils_source: str) -> None:
    body = function_body(
        utils_source, "pentax_companion_collision_probe_allows_publication"
    )
    # Exactly one accepted value. Any additional equality widens the gate back to the
    # fail-open behaviour this PR removes.
    require(
        "GP_ERROR_FILE_NOT_FOUND" in body,
        "predicate no longer accepts GP_ERROR_FILE_NOT_FOUND",
    )
    for forbidden in ("GP_ERROR_BAD_PARAMETERS", "GP_ERROR_DIRECTORY_NOT_FOUND"):
        require(
            forbidden not in body,
            f"predicate must not treat {forbidden} as an absent slot; "
            "BAD_PARAMETERS means NULL arguments (caller bug), and "
            "DIRECTORY_NOT_FOUND is not proven absence of the name",
        )
    require(
        body.count("return") == 1,
        "predicate should have a single unambiguous return",
    )


def check_call_sites(library_source: str) -> None:
    marker = "pentax_companion_collision_probe_allows_publication"
    sites = [i for i in range(len(library_source)) if library_source.startswith(marker, i)]
    require(len(sites) >= 4,
            f"expected at least 4 guarded collision probes, found {len(sites)}; "
            "a call site may have lost its fail-closed guard")

    # Each guard must be followed by a return of the probe's own error, not a fallthrough.
    for index in sites:
        window = library_source[index:index + 600]
        require(
            "return existing" in window or "ret = existing" in window,
            "a collision probe guard does not propagate its error; "
            "the candidate could be published over an existing file",
        )

    # The two retry loops must both go through the shared formatter. The repo uses GNU
    # style (a space before the argument list), so match either spelling.
    helper_calls = sum(
        library_source.count(f"pentax_collision_suffix_name{sp}(") for sp in ("", " ")
    )
    require(
        helper_calls >= 2,
        f"primary and companion collision retries must share pentax_collision_suffix_name() "
        f"(found {helper_calls} call sites, expected >= 2)",
    )

    # No in-place extension aliasing may return to either retry path.
    for signature in ("camera_pentax_capture_internal",):
        body = function_body(library_source, signature)
        require(
            "dot = strrchr (path->name" not in body or "original_name" in body,
            "collision retry appears to write into the buffer strrchr() read from; "
            "snprintf source and destination must not overlap",
        )


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: test-pentax-collision-failclosed.py <library.c> [pentax-utils.c]",
              file=sys.stderr)
        return 2
    library_path = sys.argv[1]
    utils_path = sys.argv[2] if len(sys.argv) > 2 else None
    try:
        with open(library_path, encoding="utf-8") as handle:
            library_source = handle.read()
    except OSError as error:
        print(f"SKIP: cannot read {library_path}: {error}")
        return 77

    if utils_path is None:
        from os.path import dirname
        utils_path = f"{dirname(library_path)}/pentax-utils.c"
    try:
        with open(utils_path, encoding="utf-8") as handle:
            utils_source = handle.read()
    except OSError as error:
        print(f"SKIP: cannot read {utils_path}: {error}")
        return 77

    try:
        check_predicate(utils_source)
        check_call_sites(library_source)
    except AssertionError as error:
        print(f"FAIL: {error}")
        return 1

    print("PASS: Pentax collision probes are fail-closed and share one suffix formatter")
    print("NOTE: stateful transfer/delete counters (review cases 2,4,5) are NOT covered "
          "here; they need an injected-filesystem harness that does not exist yet.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
