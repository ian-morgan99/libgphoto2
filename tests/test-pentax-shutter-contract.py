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
        print("usage: test-pentax-shutter-contract.py <config.c> <pentax-utils.c>",
              file=sys.stderr)
        return 2
    config = open(sys.argv[1], encoding="utf-8").read()
    utils = open(sys.argv[2], encoding="utf-8").read()

    shutter = function_body(config, "_put_Pentax_DirectShutter (CONFIG_PUT_ARGS)")
    verifier = strip_comments(function_body(config, "_pentax_verify_property_value (PTPParams *params,"))
    conditions_verifier = strip_comments(function_body(
        config, "_pentax_verify_rational_in_conditions (PTPParams *params,"))
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
    # offsets 272/276 in Bulb is exactly what produced the false pass.  The bit
    # must be re-checked on every fresh sample inside the verifier, not once from
    # the caller's pre-write snapshot: the dial can move to B mid-window, and a
    # requested 30/1 would then match the bulb timer and return GP_OK.
    require("PENTAX_CONDITION_CAN_CHANGE_TV" in shutter,
            "the conditions fallback must stay gated on CAN_CHANGE_TV")
    require("_pentax_cond_shutter_den,\n\t\t\t\t\tPENTAX_CONDITION_CAN_CHANGE_TV)"
            in shutter,
            "the shutter conditions fallback must pass CAN_CHANGE_TV to the verifier")
    require("required_capability" in conditions_verifier,
            "the conditions verifier must take a required-capability argument")
    # The guard must be one live expression: testing the bit against each fresh
    # sample AND'd with the caller's requirement.  Asserting the two halves
    # separately would still pass if the test were short-circuited away.  Match
    # on whitespace-collapsed source so re-indentation cannot defeat it.
    collapsed = " ".join(conditions_verifier.split())
    require("if (required_capability && !(conditions.capability_flags & required_capability))"
            in collapsed,
            "the conditions verifier must test the required capability against "
            "each fresh sample, not the caller's pre-write snapshot")
    require("capability_lost = 1" in conditions_verifier,
            "losing the capability mid-window must be recorded, never accepted")
    require(conditions_verifier.index("capability_lost = 1") <
            conditions_verifier.index("return GP_OK"),
            "the capability check must precede any acceptance")
    require("if (capability_lost)" in conditions_verifier,
            "a lost capability must be reported as unconfirmed, not success")

    # #27: the actual state must reach the caller, not only the log.  A caller
    # with an error callback but no log callback would otherwise see a bare
    # GP_ERROR.  Both terminal failure paths must report through the context, so
    # two calls are required and no GP_LOG_E may remain in either verifier.
    require(verifier.count("gp_context_error") >= 2,
            "both verification failure paths (no read-back, and mismatch) must "
            "report through gp_context_error")
    require("GP_LOG_E" not in verifier,
            "verification failures must not be log-only: a caller with an error "
            "callback but no log callback would see only GP_ERROR")
    require(conditions_verifier.count("gp_context_error") >= 2,
            "both conditions verification failure paths must report through "
            "gp_context_error")
    require("GP_LOG_E" not in conditions_verifier,
            "conditions verification failures must not be log-only either")

    # The gate judges Bulb/seconds requests against the timer domain the body
    # advertises for 0xd00f (K-3 III: 1-600 s, 300->600->300 write proven), not
    # against the 13-entry list synthesised for an empty enumeration.
    require("advertised_timer_min" in gate and "advertised_timer_max" in gate,
            "the gate must take the advertised timer domain")
    require("numerator >= advertised_timer_min" in gate and
            "numerator <= advertised_timer_max" in gate,
            "the gate must compare the request against the advertised bounds -- "
            "300 and 600 s are hardware-proven writable but absent from the "
            "13-entry synthesis subset")
    require("pentax_shutter_value_is_bulb_seconds (requested)" in gate,
            "the published subset must remain the fallback when no domain was read")
    require(gate.index("advertised_timer_min == 0") <
            gate.index("numerator >= advertised_timer_min"),
            "the advertised domain must win whenever it is known")
    require("_pentax_advertised_bulb_timer_domain (&desc, &timer_min, &timer_max)"
            in shutter,
            "shutter put must read the advertised timer domain from the descriptor")
    require("PTP_DPFF_Range" in function_body(
        config, "_pentax_advertised_bulb_timer_domain (const PTPDevicePropDesc *desc,"),
        "the advertised domain must handle a Range form descriptor")
    require("den != 1" in function_body(
        config, "_pentax_advertised_bulb_timer_domain (const PTPDevicePropDesc *desc,"),
        "the advertised domain must ignore non-timer (1/N) entries")

    # --- Change 3: unmeasured exposure-mode codes must not get invented names. ---
    naming = function_body(utils, "pentax_exposure_mode_name (uint32_t exposure_mode)")
    require("default:" in naming and "return NULL;" in naming,
            "unmeasured exposure-mode codes must return NULL, not a guess")

    print("pentax shutter gate/verification contract OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
