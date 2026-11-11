#!/usr/bin/env python3
"""Compile the real Pentax held-Bulb action in a fixture with mocked transport.

Issue #140: commit 4fba11d81 added a gp_context_cancel() guard to the deferred
camera-timed Bulb fallback, and tests/test-pentax-capture-contract.py only asserts
source substrings and ordering. Static checks cannot show that cancellation actually
returns GP_ERROR_CANCEL without writing a shutter, or that pending state is cleared so
the next capture is permitted. This extracts the production function verbatim -- no
duplicated logic, no test-only hook in the camlib -- so the real control flow runs
against an instrumented transport.

Extracted from camlibs/ptp2/library.c:
  * the PentaxCaptureMode typedef, emitted verbatim rather than re-declared, so the
    harness cannot drift from the production mode values;
  * translate_ptp_result, so PTP errors map the way they do in the driver;
  * ptp2_pentax_bulb_action itself, the code under test.

pentax_monotonic_ms is deliberately NOT extracted: the harness supplies a scripted
clock so the measured Bulb hold -- and therefore the shutter value written on the
non-cancel path -- is exact rather than dependent on wall-clock rounding.
"""
import pathlib
import re
import runpy
import sys

extract = runpy.run_path(str(pathlib.Path(__file__).with_name(
    "extract-camera-exit.py")))["extract"]
source = pathlib.Path(sys.argv[1]).read_text(encoding="utf-8")

# The mode enum verbatim from production. A regex rather than extract() because a
# typedef has no body to balance, and copying the values by hand is exactly the kind
# of drift that makes a harness pass while testing the wrong thing.
enum = re.search(r'typedef enum \{.*?\} PentaxCaptureMode;', source, re.S)
if not enum:
    raise SystemExit("PentaxCaptureMode typedef not found; has it been renamed?")

functions = [
    enum.group(0),
    extract(source, "int\ntranslate_ptp_result (uint16_t result)"),
    extract(source, "int\nptp2_pentax_bulb_action (Camera *camera, int enabled,"
                    " GPContext *context)"),
]
pathlib.Path(sys.argv[2]).write_text(
    "/* Generated from library.c; do not edit. */\n" +
    "\n\n".join(functions) + "\n", encoding="utf-8")
