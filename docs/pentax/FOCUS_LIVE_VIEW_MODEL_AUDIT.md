# Pentax focus and PC live-view model audit

Status: source audit completed 2026-10-28; physical qualification remains
model- and firmware-specific.

This document is the implementation record for GitHub issues [#134](https://github.com/ian-morgan99/libgphoto2/issues/134)
and [#135](https://github.com/ian-morgan99/libgphoto2/issues/135). It is
deliberately split into source evidence, libgphoto2 behaviour, and hardware
evidence. A successful PTP response is not physical focus or live-view
qualification.

## Source used

The bundled IMAGE Transmitter 2 decompilation is the normative source:

| IT2 member | What it proves |
|---|---|
| `RemoteAssistant/MtpDevice.cs`, `Model` setter | Model number, extension version, transfer family, focus family, movie flags, dual-slot flags, high-resolution PC-LV flag, and the explicit K-3 III Monochrome and G900SE branches. |
| `PcLiveViewMode` | Ordinary PC-LV sequence: read `0xd009` as a 20-byte geometry record, set `0xd035` to one, poll `0x9006`, stop polling, then set `0xd035` to zero. Stop accepts `0xa005` or `0x2001`. There is no model gate in this setter beyond vendor mode. |
| `FocusFineTune` | Old-focus `0x9016` with amount `abs(value * 5)` and direction `0` for the UI Far button (`value=+1`) or `1` for the UI Near button (`value=-1`). New-focus `0x9017` uses `-(openAvNum * 2.5 / 3.0) * value` and retries `0xa00c` with a bounded square-root escalation. |
| `CamAutoFocus` | AF-only `0x9011` with focus mode `1`, gated by live AF mode and shutter/movie state, but not by a model-specific branch or an explicit PC-LV property check. |
| `TouchAF` | `0x9011` with focus mode `1` or `3`, focus-area mode `3`, depending on `TouchAFSwitch`. This is distinct from ordinary AF and from manual focus. |
| `LiveViewAFPosition` and `FocusPeakingMode` | `0xd036` uses an 8-byte `{2,0,0,0,xlo,xhi,ylo,yhi}` payload; `0xd02b` is a one-byte focus-peaking setting. IT2 keeps focus position client-side when camera read-back is not an exact echo. |
| `IsReleaseWithAF` | Maps the client policy to `0x9011` focus mode `3` or `2` during the release path. It is not the same setting as `TouchAFSwitch`. |

The source has no operation named StarAF. The reported StarAF startup crash is
therefore a lifecycle/integration symptom until a direct libgphoto2 trace
identifies a lower-level operation.

## Complete IT2 model reconciliation

`New` and `Old` below mean the IT2 focus protocol, not a claim that the body
has been physically qualified. `Preview path` describes the current
libgphoto2 `capture_preview` implementation. `Widget` describes the current
public `pentaxpclvmode` gate, which is intentionally narrower than the IT2
ordinary PC-LV setter while the descriptor behaviour is unresolved.

| IT2 model | IT2 ID / focus | Current libgphoto2 mapping | Preview path | PC-LV/high-res status | Audit result |
|---|---:|---|---|---|---|
| K-3 Mark III Monochrome | 78550 / New | PID `0x018f`, now distinct from colour K-3 III | Preliminary `d035` read | High-res capability is source-backed; public widget follows K-3 III gate | `EXACT` model/focus family after this audit; no physical evidence |
| K-3 Mark III | 78420 / New | PID `0x0189` | Preliminary `d035` read | High-res capability is source-backed; public widget enabled | `MODEL_GATING_MISMATCH` for the ordinary IT2 start sequence; preserve until direct K-3 comparison is run |
| K-3 | 77760 / Old | PID `0x0165` | Preliminary `d035` read | No high-res branch in IT2 | Focus family exact from IT2; LV hardware unproven |
| 645Z | 77840 / Old | PID `0x0167` | Preliminary `d035` read | No high-res branch in IT2 | Focus family exact from IT2; LV hardware unproven |
| 645D | 77320 / Old | PID `0x0130`, extension 0 | Preliminary `d035` read | No high-res branch in IT2 | Focus family exact from IT2; LV hardware unproven |
| K-1 Mark II | 78400 / Old | PID `0x0183` (MSC `0x0182` is not PTP control) | Direct `d035=1`, matching IT2 | No high-res branch in IT2; `pentaxpclvmode` remains withheld because its GET is unusable on the tested body | Direct-start path matches; physical focus is not qualified |
| K-1 | 77970 / Old | PID `0x0179` | Direct `d035=1`, matching IT2 | No high-res branch in IT2; widget gate remains withheld | Direct-start path matches; hardware untested |
| KP | 78380 / New | PID `0x017f` | Preliminary `d035` read | No high-res branch in IT2 | Focus family exact from IT2; hardware untested |
| K-70 | 78370 / Old | PID `0x017d` | Preliminary `d035` read | No high-res branch in IT2 | Focus family exact from IT2; hardware untested |
| KF | 78520 / Old | PID `0x018e` | Preliminary `d035` read | No high-res branch in IT2 | IT2 **does contain** a KF branch; the old issue wording was stale |
| GR III | 78350 / New | PID `0x210f` | Preliminary `d035` read | No high-res branch in IT2 | Focus family exact from IT2; hardware untested |
| G900SE | 78460 / Old | No current libgphoto2 Pentax lookup/ability row | Not implemented | IT2 supports it as an LS/Ricoh model | `MISSING`; do not silently treat it as a Pentax DSLR |

K-3 II is intentionally outside this table: the checked IT2 source does not
contain a K-3 II model branch, so the current library fails closed for it.

## Live-view path findings (#134)

### What matches

- `0xd035` is a one-byte value and `0x9006` is the live-view frame operation.
- K-1 and K-1 Mark II use the direct-start path in `camera_capture_preview`,
  matching the IT2 setter's absence of a preliminary `d035` read.
- Preview frames are bounded, validated as complete JPEGs, and followed by
  restoration of the original PC-LV state. This is the capture-preview path,
  not proof that every public configuration widget is available.

### What does not yet match

- IT2 reads `0xd009` before starting ordinary PC-LV. `capture_preview` does not
  currently perform that geometry read because it only needs a frame. The
  separate `pentaxliveviewcontrols` and AF-position handlers do read and parse
  the geometry record.
- IT2's ordinary `PcLiveViewMode` setter has no K-3-only model gate. The current
  `pentaxpclvmode` widget and `autofocusdrive` handler use
  `pentax_model_supports_pc_live_view`, which currently admits only the K-3 III
  family. This is a deliberate fail-closed integration boundary, not evidence
  that IT2 lacks ordinary PC-LV on the other recognised models.
- The current preview path retains the observed-state preliminary read for K-3
  III and other non-K-1 bodies. That differs from the source setter and must
  be resolved by a direct, model-by-model trace before changing the proven K-3
  path.
- Movie-mode cleanup is not yet a complete per-model contract. IT2 explicitly
  clears its local PC-LV state when a K-1-family body enters movie mode; the
  library needs a direct test of the equivalent state transition and reconnect
  cleanup.

### Required next direct tests

For each attached or acquired body, record firmware, VID:PID, exact library
SHA, initial `d035`, `d009` length/fields, start response, first `0x9006`
response, JPEG validity, stop response, restored `d035`, reconnect, and movie
transition. Do not use a downstream Polaris failure as a library result.

## Focus path findings (#135)

### Model/family mapping

The capability table now preserves the IT2 distinction between colour K-3 III
`78420` and Monochrome `78550`. The generic public controls dispatch as follows:

| Public control | New-focus models | Old-focus models | IT2 comparison |
|---|---|---|---|
| `manualfocusdrivenear` / `manualfocusdrivefar` | `0x9017`, displacement derived from live `openAvNum` | `0x9016`, amount 5 and source-faithful direction | Same family split as `FocusFineTune` |
| `manualfocusdrive` | Same `0x9017` path, range `-7..7` | Same `0x9016` path, amount 5 per request | Same public name across models; magnitude policy is libgphoto2's bounded API contract |
| `autofocus` | Capture-time AF policy | Capture-time AF policy | Separate from `CamAutoFocus` |
| `autofocusdrive` | Current code only exposes K-3 III-family widget | Current code does not expose it | Current gate is narrower than IT2 `CamAutoFocus`; deliberate open item |
| `manualfocus` | Not exposed as AF/MF setter | Not exposed as AF/MF setter | No source-faithful generic setter is currently established |
| Touch AF / release-with-AF | Not exposed as one public widget | Not exposed as one public widget | `TouchAF`, `TouchAFSwitch`, and `IsReleaseWithAF` are separate IT2 paths |

The sign/direction conversion is now source-faithful:

- public `+1` means Near and `-1` means Far;
- IT2 UI Near sends `value=-1`, so old-focus `0x9016` direction is `1` and
  new-focus `0x9017` displacement is positive;
- IT2 UI Far sends `value=+1`, so old-focus direction is `0` and new-focus
  displacement is negative;
- `openAvNum * 5 / 6` is the integer equivalent of IT2's
  `openAvNum * 2.5 / 3.0` calculation.

The historical hardware log records protocol acknowledgements, preview
continuity, and cleanup, but not observed lens movement. Its earlier direction
comment must not be treated as physical proof. A bounded physical Near/Far
test remains required before claiming focus support for any body.

### AF and “AF With Live View”

The current `autofocusdrive` implementation sends `0x9011` with focus mode 1,
but additionally requires a successful `d035` read and active PC-LV. IT2's
`CamAutoFocus` instead checks live `AFMode` and shutter/movie state and then
sends the same operation without a model-specific or explicit PC-LV gate. This
is a real contract difference and is not closed by the K-3 III result.

The safe next step is a direct library contract decision and per-model tests,
not widening the gate based on an ACK. In particular, distinguish:

1. immediate AF (`0x9011`, focus mode 1);
2. touch AF (`0x9011`, focus-area mode 3, focus mode 1 or 3);
3. capture-time AF policy (`autofocus` / `IsReleaseWithAF`); and
4. StarAF, for which the checked IT2 source has no named operation.

## Status labels and release rule

- `EXACT`: source mapping and current code path agree; hardware may still be
  `UNPROVEN`.
- `MODEL_GATING_MISMATCH`: source has a path, but current public exposure or
  start/stop gate is narrower or different.
- `WRONG_PAYLOAD` / `WRONG_DIRECTION`: direct source comparison found a code
  or sign error; the source-faithful unit test must be merged before hardware
  retest.
- `MISSING`: IT2 or the current consumer has a model/control that has no
  current libgphoto2 path.
- `UNPROVEN`: source mapping exists but no direct library or physical evidence
  exists for that exact body and firmware.

Do not close #134 or #135 from unit tests, a `0x2001` response, a downstream
Benro run, or a successful frame alone. Close only when the source matrix,
direct library trace, hardware restoration record, and exact merge commits are
all linked from the issue.
