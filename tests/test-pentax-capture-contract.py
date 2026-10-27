#!/usr/bin/env python3
"""Keep the K-3 III ordinary and experimental capture routes separate.

This is a source-contract test, not a hardware qualification test.  The
ordinary path is deliberately small and explicit because a future refactor
must not silently route Manual or camera-timed Bulb through release mode 2.
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


def fallback_region(source: str) -> str:
    """The camera-timed Bulb fallback: shutter write through the timed capture.

    Extracted so a contract can be asserted over *this* stretch alone -- the
    place where .75 inserted an early return and hung every capture.
    """
    action = function_body(source, "ptp2_pentax_bulb_action (Camera *camera")
    start = action.index("ptp_setdevicepropvalue (params, PTP_DPC_PENTAX_ShutterSpeed")
    # End before the capture itself: the fallback's own `return
    # camera_pentax_capture_internal (...)` is the required outcome, not an early
    # exit.  Anything returning inside this stretch is a refusal.
    end = action.index("memset (&path", start)
    return action[start:end]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: test-pentax-capture-contract.py <library.c>", file=sys.stderr)
        return 2
    source = open(sys.argv[1], encoding="utf-8").read()

    timed = function_body(source, "pentax_initiate_camera_timed_capture")
    held = function_body(source, "pentax_initiate_experimental_held_capture")
    wrapper = function_body(source, "camera_pentax_capture (Camera *camera")
    action = function_body(source, "ptp2_pentax_bulb_action (Camera *camera")
    internal = function_body(source, "camera_pentax_capture_internal (Camera *camera")

    require("ptp_pentax_initiate_capture (params, 0U" in timed,
            "timed K-3 III route must use release mode 0")
    require("ptp_pentax_initiate_capture (params, 2U" in held,
            "experimental held route must be visibly distinct")
    require("PENTAX_CAPTURE_TIMED" in wrapper,
            "ordinary camera wrapper must select the timed route")
    require("ptp_pentax_terminate_capture (params," in action,
            "explicit Bulb stop must be the owner of TerminateCapture")
    require("params->pentax.recovery_required ||" not in action,
            "Bulb start must reach the shared strict recovery probe")
    require("camera_pentax_capture_internal (camera, NULL, context" in action and
            "PENTAX_CAPTURE_BULB_START" in action,
            "Bulb start must use the shared capture admission/lifecycle path")
    # The shared cleanup path may abort an errored, pre-candidate operation.
    # A successful natural timed completion returns before that error-only
    # branch; the explicit action stop remains the only normal Bulb stop edge.
    require("ret = GP_OK;" in internal and
            "ptp_pentax_terminate_capture (params," in internal,
            "errored timed captures must retain bounded abort cleanup")
    require("pentax_initiate_camera_timed_capture (params, focus_mode)" in internal,
            "ordinary capture must call the explicit release=0 helper")
    require("pentax_initiate_experimental_held_capture (params" in internal,
            "held research action must remain an explicit separate branch")
    require("bulb_action_start ? 2U : 0U" not in internal,
            "ordinary capture must not use a conditional release-mode shortcut")

    # --- camera-timed Bulb fallback must always reach the capture (issue #239)
    # .75 made this stretch fail closed: a failed shutter write returned a mapped
    # PTP error, and a new GetDevicePropValue readback refused on mismatch. Neither
    # Benro Connect nor pgphoto surfaces a capture error -- they wait for a file that
    # never arrives -- so every timed capture spun forever. Operator verdict:
    # "objectively worse than .74". Reverted in df9d72745; these checks keep it out.
    fallback = fallback_region(source)
    require("ptp_getdevicepropvalue" not in fallback,
            "camera-timed fallback must not read back PTP_DPC_PENTAX_ShutterSpeed: "
            "GetDevicePropValue is not a supported read path for this property "
            "(config.c reads it only via GetDevicePropDesc CurrentValue)")
    require("translate_ptp_result (setres)" not in fallback,
            "camera-timed fallback must not fail closed on the shutter write: the "
            "caller cannot surface an error and will spin waiting for a file")
    require("no capture was started" not in fallback,
            "camera-timed fallback must not refuse to capture")
    # The only return in the fallback is the timed capture itself. .75 had three
    # early returns here; any new one re-creates the spinner.
    returns = [line.strip() for line in fallback.splitlines() if line.strip().startswith("return ")]
    require(len(returns) == 0,
            f"camera-timed fallback must not return before the timed capture; found: {returns}")
    action_full = function_body(source, "ptp2_pentax_bulb_action (Camera *camera")
    tail = action_full[action_full.index("ptp_setdevicepropvalue (params, PTP_DPC_PENTAX_ShutterSpeed"):]
    require("return camera_pentax_capture_internal (camera, &path, context," in tail
            and "PENTAX_CAPTURE_TIMED" in tail,
            "camera-timed fallback must reach camera_pentax_capture_internal")

    print("test-pentax-capture-contract: all checks passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, OSError, ValueError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
