#!/usr/bin/env python3
"""Compile the production result-code translator without a camera.

translate_ptp_result decides what every PTP failure looks like to a client, so
its mapping is worth pinning down with a test that runs in CI. Extracting the
real body keeps this honest: it cannot drift into asserting a copy of the logic.
"""
import pathlib
import runpy
import sys

extract = runpy.run_path(str(pathlib.Path(__file__).with_name(
    "extract-camera-exit.py")))["extract"]
source = pathlib.Path(sys.argv[1]).read_text(encoding="utf-8")
body = extract(source, "int\ntranslate_ptp_result (uint16_t result)")
pathlib.Path(sys.argv[2]).write_text(
    "/* Generated unchanged production body; no hardware. */\n" + body + "\n",
    encoding="utf-8")
