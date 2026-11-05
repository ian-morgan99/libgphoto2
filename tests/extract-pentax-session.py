#!/usr/bin/env python3
"""Compile the production observer, recovery and OpenSession loop in a fixture."""
import pathlib
import runpy
import sys

extract = runpy.run_path(str(pathlib.Path(__file__).with_name(
    "extract-camera-exit.py")))["extract"]
source = pathlib.Path(sys.argv[1]).read_text(encoding="utf-8")
functions = [extract(source, signature) for signature in (
    "int\ntranslate_ptp_result (uint16_t result)",
    "static int\npentax_reconcile_reused_session (PTPParams *params, GPContext *context)",
    "static int\npentax_failed_session_init (Camera *camera, int result)",
    "static int\npentax_recover_stale_session (Camera *camera, GPContext *context, uint32_t sessionid)",
)]
init = extract(source, "int\ncamera_init (Camera *camera, GPContext *context)")
loop = extract(init, "\ttries = 0;\n\tsessionid = 1;\n\twhile (1)")
functions.append("""static int tested_open_session (Camera *camera,
    GPContext *context, int pentax_candidate)
{
    PTPParams *params = &camera->pl->params;
    int ret, tries;
    uint32_t sessionid;
""" + loop + "\n    return GP_OK;\n}\n")
pathlib.Path(sys.argv[2]).write_text(
    "/* Generated unchanged production bodies; no hardware. */\n" +
    "\n\n".join(functions), encoding="utf-8")
