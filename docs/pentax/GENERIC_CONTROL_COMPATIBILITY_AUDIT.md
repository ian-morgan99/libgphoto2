# Pentax/Ricoh generic control compatibility audit

Revision: 2026-09-08

## Purpose

This document records a cross-layer audit prompted by Benro Polaris `pgphoto` failures where controls already implemented for Pentax were not discoverable through the generic libgphoto2 widget names used by third-party clients.

The key finding is that this is not simply a Benro-specific compatibility problem. The legacy `camlibs/pentax` driver already established a Pentax-facing vocabulary including `imageformat`, `iso`, `shutterspeed`, `aperture`, and `bulb`, while the newer Pentax support in `camlibs/ptp2` sometimes exposes equivalent or related functionality under different names such as `f-number`, `pentaxdirectaperture`, `manualfocusdrivenear/far`, `oldfocusdrivenear/far`, `pentaxpclvmode`, and `pentaxcardwritingmode`.

Generic aliases or wrappers should therefore be treated as libgphoto2 API compatibility work, not as Benro-specific hacks. Existing Pentax-specific controls must remain available for backward compatibility and diagnostics.

## Control audit

| Generic control | Current Pentax situation | Required disposition |
|---|---|---|
| `iso` | Already exposed generically on modern ptp2 and legacy Pentax | Preserve; regression-test across model families |
| `shutterspeed` | Already exposed generically on modern ptp2 and legacy Pentax | Preserve; re-verify K-1 II |
| `aperture` | Legacy Pentax exposes `aperture`; modern ptp2 exposes `f-number` and `pentaxdirectaperture` | Add compatibility alias/wrapper with identical value semantics |
| `imageformat` | Legacy Pentax exposes JPEG/RAW/RAW+JPEG; modern ptp2 has Pentax `0xd01b WritingFileFormat` but no generic widget | Implement model-gated `imageformat`; do not confuse with card-writing mode |
| `imagequality` | Requested by pgphoto; not currently represented as a generic modern Pentax widget | Determine the correct Pentax property/compound field before implementation |
| `autofocus` | Pentax capture code already reads the `ptp2/autofocus` setting, but the Pentax config surface does not clearly expose the normal virtual policy widget | Add/verify the generic capture-policy widget independently of focus-drive operations |
| `autofocusdrive` | Missing for Pentax | Implement a real immediate-AF action if protocol semantics are established; never alias to manual near/far drive |
| `manualfocusdrive` | Pentax has directional model-specific drive widgets (`0x9017` new family; `0x9016` old family) | Add a generic wrapper preserving direction/step semantics and model dispatch |
| `manualfocus` | Requested by pgphoto; distinct from lens-drive commands | Establish whether this means AF/MF mode selection on supported Pentax models before exposing it |
| `capturetarget` | Missing; pgphoto attempts `capturetarget=1` | Define honest Pentax semantics or make clients tolerate absence; do not invent an SDRAM/card mapping without protocol evidence |

## Critical semantic constraints

1. `pentaxcardwritingmode` is a card/slot-writing bitmask. It is **not** JPEG/RAW image format and must not back `imageformat`.
2. `autofocus`, `autofocusdrive`, and `manualfocusdrive` are separate concepts in libgphoto2. Do not implement one as an alias of another.
3. The K-3 III uses the newer focus family (`0x9017`), while the K-1 II and several older modern-vendor bodies use the older focus family (`0x9016`). Generic wrappers must dispatch by model capability, not by one global Pentax implementation.
4. The K-01 must be treated as two separate USB paths: legacy `25fb:0130` USB-SCSI via `camlibs/pentax`, and `25fb:0131` generic PTP/MTP via `ptp2`. The latter has hardware evidence of Microsoft MTP DeviceInfo with no Pentax vendor operations and must not inherit modern Pentax vendor-mode claims.
5. Compatibility work must add aliases/wrappers without removing the existing Pentax-specific diagnostic/config names.

## Model-family sweep scope

The audit must cover every Pentax/Ricoh body referenced by the current research corpus, not only the three cameras physically used most often. At minimum the sweep should include:

- K-3 III and K-3 III Monochrome
- K-1 II and K-1
- K-3 and K-3 II
- KP
- K-70
- KF
- 645D and 645Z
- K-01 (both USB modes/path identities)
- GR III
- legacy bodies already represented by `camlibs/pentax`, including K20D, K10D, K100D/K100D Super, K200D, K-5-family, K-30, K-50 and related models present in the driver tables
- Ricoh/Pentax-branded PTP models present in `camlibs/ptp2` or referenced in the NINA/Pentax capability research

For every model record: USB VID:PID by mode, claiming camlib, vendor-extension identity, model-number/capability family where known, generic widget names present, Pentax-specific widgets present, capture path, preview/live-view path, focus family, file-format controls, capture-target semantics, direct hardware evidence, firmware/decompilation evidence, and whether support is implemented, inferred, fail-closed, or hardware-verified.

Do not claim a body works solely because another body shares a protocol family.

## Tracking model

Use one umbrella model/control matrix issue plus one independently closeable issue per generic control or semantic capability. A child issue should contain its own protocol source, implementation plan, model gates, unit/regression tests, and direct-hardware acceptance criteria. This allows agents to work in parallel without treating a partial compatibility fix as completion of the whole Pentax surface.

The hardware-validation matrix in issue #44 remains the authority for claims of real-camera support. This compatibility audit supplements it; it does not weaken the requirement for direct evidence.

## Related research

- #51: discovery/compatibility umbrella for generic names observed in Benro pgphoto traffic.
- #50: K-01 mode/driver split and limitations of `25fb:0131` MTP/PTP.
- #44: direct Pentax hardware validation matrix.
- #42: upstream-style isolation and maintainability audit.
- `docs/pentax/IMAGE_TRANSMITTER_CAPABILITY_MATRIX.md`: model/protocol source-of-truth for modern Image Transmitter-derived behaviour.
- `docs/pentax/CAPABILITY_MATRIX_AUDIT.md`: prior model-capability safety audit.

## Completion criterion

The programme is complete only when every referenced Pentax/Ricoh model has an explicit row and every generic control requested by downstream clients is classified as one of: supported and tested, supported through a compatibility alias/wrapper, intentionally unsupported with documented semantics, or unverified/fail-closed. No unknown/missing widget should remain unexplained.
