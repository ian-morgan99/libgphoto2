#!/usr/bin/env python3
"""Fail closed on an empty enumeration instead of publishing a dead control.

A source-contract test, not a hardware qualification test.

1. Exposure compensation (patcher #269), measured on a live K-3 III
   (USB 25fb:0189) on 2026-11-05: exposurecompensation returned a widget with
   choices=0, reproducibly 3/3, while iso/aperture/whitebalance/shutter all
   returned populated lists in the same session.  Benro's Stage-2 guard rejects
   an empty choice list with CAMERA_BUSY, which turned every EV poll into a
   permanent failure on an otherwise healthy body.

   The descriptor shape is pinned by deduction, not by a captured
   GetDevicePropDesc: _get_ExpCompensation returns GP_ERROR unless FormFlag is
   Enumeration and DataType is INT16, so a widget that was created at all must
   have had both, leaving NumberOfValues == 0 as the only remaining explanation
   for zero choices.  Capture the descriptor on the body before promoting this
   to a confirmed protocol claim.

2. Unchecked cache re-fetch (related to patcher #176).  The K-3 III re-sends
   ObjectAdded for an object libgphoto2 has already consumed.  In the committed
   transcript the walk fails one step earlier -- ptp_object_want returns PTP
   Invalid Object Handle (0x2009) and C_PTP propagates it, so the event is
   dropped cleanly with -1.  append_folder_from_handle nevertheless called
   ptp_find_object_in_cache without checking its result, and that function
   assigns the bsearch result to *retob before testing it: a miss leaves the
   pointer NULL and the next line dereferences it.  find_object_path wraps the
   identical call in CR(); this one did not.  This is a defensive consistency
   fix, not a reproduction of the #176 crash, and must not be described as one.

A refactor must not silently restore either behaviour, and neither can be caught
by a unit test that has no camera.
"""

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
    raise AssertionError(f"unterminated function: {signature}")


def strip_comments(source: str) -> str:
    """Drop C comments so an assertion cannot be satisfied -- or defeated -- by
    explanatory prose that merely mentions the token it is looking for."""
    out = []
    i = 0
    while i < len(source):
        if source.startswith("/*", i):
            end = source.find("*/", i + 2)
            i = len(source) if end < 0 else end + 2
        elif source.startswith("//", i):
            end = source.find("\n", i)
            i = len(source) if end < 0 else end
        else:
            out.append(source[i])
            i += 1
    return "".join(out)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: test-pentax-empty-descriptor-contract.py <config.c> <library.c>",
              file=sys.stderr)
        return 2
    config = open(sys.argv[1], encoding="utf-8").read()
    library = open(sys.argv[2], encoding="utf-8").read()

    ev = strip_comments(function_body(config, "_get_ExpCompensation(CONFIG_GET_ARGS)"))
    walk = strip_comments(function_body(library, "append_folder_from_handle (Camera *camera,"))

    # --- Change 1: an empty enumeration is not a usable control. ---
    require("NumberOfValues == 0" in ev,
            "_get_ExpCompensation must reject an enumeration with no values")
    require("GP_ERROR_NOT_SUPPORTED" in ev,
            "an empty EV enumeration must fail closed as NOT_SUPPORTED, not GP_ERROR")
    # The rejection has to happen before the widget is created, otherwise the
    # zero-choice widget is still published and Benro still returns CAMERA_BUSY.
    guard = ev.index("NumberOfValues == 0")
    created = ev.index("gp_widget_new (GP_WIDGET_RADIO")
    require(guard < created,
            "the empty-enumeration check must precede gp_widget_new: a widget with "
            "zero choices is exactly what produces the permanent CAMERA_BUSY in #269")
    # And it must be a return, not a fall-through that still adds choices.
    between = ev[guard:created]
    require("return" in between,
            "the empty-enumeration check must return before building choices")

    # --- Change 2: the cache re-fetch result is checked before dereference.
    # Defensive consistency fix; not a reproduction of the #176 crash. ---
    refetch = walk.index("ptp_find_object_in_cache(params, handle, &ob)")
    tail = walk[refetch:]
    require(tail.startswith("ptp_find_object_in_cache") or
            "CR (ptp_find_object_in_cache(params, handle, &ob))" in walk,
            "append_folder_from_handle must check ptp_find_object_in_cache: it sets "
            "*retob to the bsearch result before testing it, so a miss yields NULL")
    deref = tail.index("ob->oi.Filename")
    require("if (!ob" in tail[:deref] or "CR (ptp_find_object_in_cache" in walk,
            "ob must be validated before ob->oi.Filename is dereferenced")

    print("test-pentax-empty-descriptor-contract: all checks passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, OSError, ValueError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
