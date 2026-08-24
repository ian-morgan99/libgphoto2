# Pentax tethering support in libgphoto2

This fork adds working USB tethering (remote control, live view, capture and
configuration) for recent Pentax bodies, developed by reviewing the vendor
opcodes and properties the cameras expose over PTP (cross-checked against
publicly observable behaviour of Ricoh's tethering software) and verified
against real hardware. It is rebased onto upstream **libgphoto2 v2.5.34**.

## What was there before

Upstream libgphoto2 has carried a minimal Pentax PTP layer for years:

- A handful of USB IDs in `camlibs/ptp2/library.c` (K3, K1, K3 II, K70, KP,
  GR III, WG-M2, plus ancient Optio models via a mass-storage camlib).
- Generic PTP property widgets (`f-number`, `whitebalance`,
  `exposurecompensation`, …) that happen to work when the camera advertises
  the standard properties.
- No vendor-mode session setup, no live view, no remote capture over the
  vendor opcodes, no focus control.

Separately, this repository previously hosted a `pentaxmodern` prototype
camlib — an early attempt at modern vendor-mode tethering built outside the ptp2
engine. That prototype has been **archived and removed**: it duplicated
transport logic that belongs in `ptp2` and could never be merged upstream.

## Why upstream didn't work for all models

Pentax bodies of this era only expose their full remote-control surface after
a **vendor-mode handshake** (a sequence of proprietary operations performed
at session start by the camera's own tethering clients). Without it:

- The camera stays in a restricted personality; live view operations
  (`0x90xx` family) are not usable.
- Several bodies (e.g. K-1 II) do not even advertise their vendor operations
  or properties in `DeviceInfo`, so libgphoto2's descriptor-driven widget
  builder culls everything Pentax-specific.
- Some properties exist on all models but are silently rejected or answered
  with an *empty data phase* on models where the camera's own UI hides the
  corresponding feature
  (verified on K-1 II hardware for bracketing, composition adjust, movie
  settings and PC live view).

So the same binary either worked partially (K-3 III) or exposed almost
nothing (K-1 II), depending on what each body chose to advertise.

## What we have done

All work lives in `camlibs/ptp2/` and `docs/pentax/`, hardware-verified per
the log in [`REAL_HARDWARE_TEST_LOG.md`](REAL_HARDWARE_TEST_LOG.md)
(~20 test sessions):

- **Vendor mode enablement** replicating the vendor session handshake.
- **Model table expansion**: K-1 Mark II (0x0183), K-3 Mark III (0x018c),
  K-3 Mark III Monochrome (0x018f), KP, K-70, K-3, K-1, GR III, 645D.
- **Live view**: PC-LV control (`0xd035`) with a `pclvkeep` toggle so
  research harnesses can hold a session open; complete-JPEG-frame validation
  and session-state restore across capture-preview calls.
- **Exposure control**: direct ISO (`pentaxdirectiso`), shutter speed
  (`pentaxdirectshutter`) and aperture writes, each verified by read-back
  through the camera's live conditions word rather than trusting an OK
  response; Bulb mode support.
- **Focus drive**: both wire generations — new-focus `0x9017` (K-3 III
  family, KP, GR III) and old-focus `0x9016`. The K-1 II does not advertise
  `0x9016` at all but honours it; `have_prop` now special-cases this, gated
  behind an AF-mode precondition to prevent wedging the lens mid-drive.
- **Per-model capability gates** mirroring the camera's own feature flags
  (`_isExpBracketSupport`, `_isCompositionAdjSupported`,
  `_isMovieSettingSupported`, `_isPcLvHighResolutionSupported`): widgets
  fail closed with `GP_ERROR_NOT_SUPPORTED` on bodies where the camera would
  return an empty data phase.
- **A raw compound-property setter** (`ptp_pentax_setdeviceprop_raw`) for
  payloads the firmware sends as opaque blobs.
- **`ptp-probe`**, a read-only diagnostic tool that compares which PTP and
  vendor opcodes different bodies advertise.

### Known limitations

- Several camera settings are **LV-gated by the firmware itself**: writes to
  bracketing mode (`0xd014`) and movie settings (`0xd039`) are only accepted
  while PC-LV is active, and revert when the vendor session closes.
- The K-1 II has display limitations over PC-P and its generic `shutterspeed`
  write path required conditions-polling verification before it could be
  trusted.
- Some capture paths remain deliberately contained/unverified until they can
  be exercised safely on hardware.

## Models we could target next

The model-matching code (`pentax_lookup_model`) is table-driven, so adding a
body is mostly a matter of its USB ID plus capability flags:

| Model | Notes |
|---|---|
| **K-3 / K-3 II** | IDs already present; old-focus generation, likely needs vendor-mode verification |
| **KP** | Already modelled (composition adjust supported); focus drive untested on hardware |
| **K-70** | ID present; no hardware access yet |
| **K-1** (original) | ID present; expected old-focus like the K-1 II |
| **GR III / GR IIIx** | GR III modelled; new-focus family per opcode review |
| **645Z** | Exposure-bracketing support indicated by capability review; no entry exists yet — first candidate if a unit appears |
| **645D** | Modelled from ID evidence only |

The blocking factor is hardware: every capability flag in this fork was set
from opcode and capability review and then confirmed (or corrected) against a
physical camera.

## What we are leaving pointing at the old mechanism

- **Generic PTP widgets** (`f-number`, `whitebalance`,
  `exposurecompensation`) still flow through the standard descriptor-driven
  path where the camera advertises them properly — no reason to duplicate.
- **Old-focus transport** remains the shared `0x9016` path used by pre-K-3
  bodies; only the *gate* was relaxed, the wire format is unchanged.
- **The archived `pentaxmodern` prototype** survives in git history
  (pre-rebase branch `backup/pre-rebase-2026-08-24` and earlier tags) as
  reference material for anyone studying the protocol; nothing builds
  against it.
- **Mass-storage personalities**: some bodies switch to MSC mode when a card
  is inserted; that behaviour is documented
  ([REAL_HARDWARE_TEST_LOG.md](REAL_HARDWARE_TEST_LOG.md)) but intentionally
  left to the existing storage stack.

## Documentation index

- [`REAL_HARDWARE_TEST_LOG.md`](REAL_HARDWARE_TEST_LOG.md) — chronological hardware test log (authoritative)
- [`IMAGE_TRANSMITTER_CAPABILITY_MATRIX.md`](IMAGE_TRANSMITTER_CAPABILITY_MATRIX.md) — vendor capability matrix reconciled with our findings
- [`PENTAX_WIRE_PROTOCOL.md`](PENTAX_WIRE_PROTOCOL.md) — vendor opcode/property reference
- [`PENTAX_CONFIGURATION.md`](PENTAX_CONFIGURATION.md) — configuration-engine integration rules
- [`IMAGE_TRANSMITTER_ERROR_RECOVERY.md`](IMAGE_TRANSMITTER_ERROR_RECOVERY.md) — session recovery sequences
- [`CAPABILITY_MATRIX_AUDIT.md`](CAPABILITY_MATRIX_AUDIT.md) — audit trail for the matrix
