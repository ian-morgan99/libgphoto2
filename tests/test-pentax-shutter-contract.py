#!/usr/bin/env python3
"""Keep the K-1 II shutter-speed gate and verification honest (issue #143).

A source-contract test, not a hardware qualification test.  Both defects it
guards were measured live on a K-1 II (USB 25fb:0183):

1. Gate.  In Bulb the body clears CAN_CHANGE_TV and sets BULB_TIMER.  The old
   (CAN_CHANGE_TV | BULB_TIMER) gate admitted the mode, the body refused the
   write, and verification burned ten cycles (~720 ms) before failing.
2. Verification.  The comparison used conditions offsets 272/276, which are the
   bulb timer in Bulb but the applied shutter speed in Manual.  A request equal
   to the existing bulb timer therefore matched a value that had nothing to do
   with the write: SET 00:30 in B returned GP_OK as a pure false pass.

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


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: test-pentax-shutter-contract.py <config.c> <pentax-utils.c>",
              file=sys.stderr)
        return 2
    config = open(sys.argv[1], encoding="utf-8").read()
    utils = open(sys.argv[2], encoding="utf-8").read()

    shutter = function_body(config, "_put_Pentax_DirectShutter (CONFIG_PUT_ARGS)")
    verifier = function_body(config, "_pentax_verify_property_value (PTPParams *params,")
    gate = function_body(utils, "pentax_shutter_write_permitted (uint32_t capability_flags,")

    # --- Change 1: the gate is a single shared predicate, not an inline OR. ---
    require("pentax_shutter_write_permitted (" in shutter,
            "shutter put must decide capability through the shared predicate")
    require("PENTAX_CONDITION_CAN_CHANGE_TV | PENTAX_CONDITION_BULB_TIMER" not in shutter,
            "the (CAN_CHANGE_TV | BULB_TIMER) OR gate must not return: it admits Bulb")
    require("PENTAX_CONDITION_CAN_CHANGE_TV | PENTAX_CONDITION_BULB_TIMER" not in gate,
            "the gate predicate itself must not OR Tv with the bulb timer")
    require("GP_ERROR_NOT_SUPPORTED" in shutter,
            "a refused shutter write must be reported as unsupported, not a generic error")

    # The predicate requires Tv for non-bulb values and keeps the bulb-timer path
    # only for actual Bulb/seconds entries.
    require("PENTAX_CONDITION_CAN_CHANGE_TV" in gate,
            "gate must test CAN_CHANGE_TV -- shutter speed is a Tv property")
    require("pentax_shutter_value_is_bulb_seconds (requested)" in gate,
            "gate must keep the bulb-timer path for Bulb/seconds entries only")
    require(gate.index("PENTAX_CONDITION_CAN_CHANGE_TV") <
            gate.index("pentax_shutter_value_is_bulb_seconds"),
            "Tv must be checked first; the bulb path is the exception, not the default")

    # Readiness still comes from the shared helper (#16), not a hand-rolled loop.
    require("_pentax_readiness_preflight (params, &conditions)" in shutter,
            "shutter put must reuse _pentax_readiness_preflight (issue #16)")

    # --- Change 2: verification reads the shutter property itself. ---
    require("PTP_DPC_PENTAX_ShutterSpeed" in shutter and
            "_pentax_verify_property_value" in shutter,
            "shutter put must verify against PTP_DPC_PENTAX_ShutterSpeed")
    require("PTP_DTC_UINT64, &value," in shutter,
            "shutter verification must read the UINT64 wire value it wrote")
    require("conditions.bulb_timer_seconds ==" not in shutter,
            "verification must not compare against the conditions bulb-timer field")
    require("bulb_timer_seconds ==" not in verifier,
            "the shared verifier must not consult a bulb-timer field either")

    # GetDevicePropValue is never served from the descriptor cache; a descriptor
    # read can return a value cached up to params->cachetime seconds ago (#27).
    require("ptp_getdevicepropvalue (params, property, &actual, datatype)" in verifier,
            "verification must use GetDevicePropValue, which is never cached")
    require("ptp_generic_getdevicepropdesc" not in verifier,
            "verification must not read the descriptor -- it can be a cached value")

    # #27: a verification failure must communicate the camera's actual state.
    require("actual_text" in verifier and "expected_text" in verifier,
            "verification failure must report both requested and actual values")
    require("requested=%s, actual=%s" in verifier,
            "verification log must carry requested and actual together")

    # A body that refuses the read-back is a capability limit, not a failed write.
    require("GP_ERROR_NOT_SUPPORTED" in verifier,
            "a refused GetDevicePropValue must be distinguishable from a mismatch")

    # The conditions fallback is only sound while Tv is changeable; trusting
    # offsets 272/276 in Bulb is exactly what produced the false pass.
    require("PENTAX_CONDITION_CAN_CHANGE_TV" in shutter,
            "the conditions fallback must stay gated on CAN_CHANGE_TV")

    # --- Change 3: unmeasured exposure-mode codes must not get invented names. ---
    naming = function_body(utils, "pentax_exposure_mode_name (uint32_t exposure_mode)")
    require("default:" in naming and "return NULL;" in naming,
            "unmeasured exposure-mode codes must return NULL, not a guess")

    print("pentax shutter gate/verification contract OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
