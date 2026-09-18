# Pentax Capture Lifecycle — Full Trace + Critical Review

Date: 2026-09-18. Source: `camlibs/ptp2/library.c` `camera_pentax_capture()`
(commit 233eafed0, o-v9r), `pentax-utils.[ch]`. Related issues: #73 (shutter
release lockup), #111 (Bulb shot-failed report), #112 (Astro 180s cap + first-shot
delay), #119 (session re-init loop), #120 (Bulb timer as pre-shot pause).

## The lifecycle, phase by phase

```
shutter release (Benro Connect / pgphoto)
  |
  v
[0] PRE-FLIGHT
    - extra_capture_count = 0
    - if recovery_required: re-probe conditions; refuse CAMERA_BUSY if still wedged
    - if transfer_state != IDLE: refuse CAMERA_BUSY
  |
  v
[1] STALE-CANDIDATE DRAIN (pre-capture)
    - read conditions; if a pending candidate handle exists from a previous
      exposure (dual-format RAW+JPEG leftovers, issue #73):
      * bounded loop: transfer each candidate to throwaway buffer + finalize
      * bounds: 8 candidates AND 60 s wall clock (DRAIN_MAX_MS)
      * then wait up to 50 x 200 ms (10 s) for activity flags to clear
    - if drain cannot clear: refuse CAMERA_BUSY (fail-closed)
  |
  v
[2] TRIGGER
    - ptp_pentax_initiate_capture(params, 0, focus_mode, 0, 0, 0)
    - transfer_state = TRIGGERED
  |
  v
[3] WAIT FOR EXPOSURE / CANDIDATE PUBLICATION
    - read conditions (3 attempts, 100 ms apart) to size the budget:
        capture_timeout_ms  = pentax_capture_timeout_ms(&conditions)   [total]
        exposure_phase_ms   = pentax_exposure_phase_ms(&conditions)    [exposure only]
      budgets (pentax-utils.c):
        base            = 60 s (PENTAX_CAPTURE_TIMEOUT_MS_BASE)
        fallback        = 120 s (if conditions unreadable, PENTAX_CAPTURE_TIMEOUT_MS_FALLBACK)
        bulb            = (bulb_timer_seconds + 1) * 1000 + margin
        multi-shot      = base or bulb x 4 (PENTAX_PIXEL_SHIFT_MULTIPLIER) + margin
        astro shift     = camera-reported astro_limit + margin
    - gp_port_set_timeout(port, capture_timeout_ms)   [raise USB timeout for long exposures]
    - poll loop: ptp_pentax_get_all_conditions until candidate handle appears
      * transient failures retried up to 5x with backoff (issue #32)
      * bounded by waiting_for_timeout(capture_timeout_ms)
    - on timeout, attribute the failure (issue #111):
        candidate_handle != 0 -> "post-exposure processing phase" timeout
        candidate_handle == 0 -> "exposure phase" timeout (budget was exposure_phase_ms)
  |
  v
[4] CANDIDATE INFO + FILENAME
    - ptp_pentax_get_transfer_candidate_info -> filename
    - preflight name collision probe (issue #37): suffix _1, _2 ... while candidate still exists
  |
  v
[5] TRANSFER
    - pentax_transfer_run: block-wise PTP data transfer of the image
    - cache into gp_file
  |
  v
[6] FINALIZE
    - ptp_pentax_delete_transfer_candidate (camera-side finalize BEFORE publish, issue #12)
  |
  v
[7] DUAL-FORMAT RECONCILIATION (issue #73)
    - pentax_reconcile_extra_candidates: consume up to 8 extra candidates
      (astro pixel-shift x4 in RAW+JPEG = up to 7 extras) within 120 s wall clock
    - each extra is published into the camera filesystem + recorded in
      params->pentax.extra_capture_files
    - failure here never fails the primary capture (already finalized); the
      next-capture pre-probe barrier covers any remainder
  |
  v
[8] POST-CAPTURE IDLE WAIT
    - poll conditions until activity flags clear AND no pending candidate
    - BOUNDED TO 60 s (IDLE_WAIT_MAX_MS)
    - if still busy after 60 s: log "camera still busy ... proceeding" and
      return success anyway; the NEXT capture's pre-probe handles it
  |
  v
[9] RESTORE
    - gp_port_set_timeout(port, normal_timeout)
    - transfer_state = IDLE
    - return GP_OK (or error)
```

## Critical review — where the lifecycle breaks

### C1. The 60 s post-capture idle wait is mode-blind (root of #112 / "waits a few minutes")

`IDLE_WAIT_MAX_MS = 60 * 1000` is a fixed constant, independent of the exposure
mode. For astro pixel-shift (4 shots + RAW conversion) or long Bulb exposures,
the camera can legitimately need >60 s to reach idle after all candidates are
consumed. The code then **proceeds as if success** and the next shutter release
fires while the camera is still busy -> `InitiateCapture` fails with
CAMERA_BUSY (exactly the #73 lockup path and the "takes a shot then waits a few
minutes" symptom in Astro mode).

The pre-capture drain (phase 1) then has to re-clear the camera, adding up to
60 s + 10 s of latency before the next exposure can fire. Net effect: a hard
~1 minute stall between shots in multi-shot/astro modes — matching the observed
"1 minute timeout before recovery with camera in manual and polaris in astro".

**Fix direction:** make the idle wait budget mode-aware, reusing the same
`pentax_capture_timeout_ms()` sizing (or at least `exposure_phase_ms + margin`)
instead of a fixed 60 s. A wedged camera is still bounded (the next-capture
pre-probe barrier remains the fail-safe).

### C2. The "countdown" the app shows is not the exposure timer (root of #120)

The libgphoto2 layer has no notion of a pre-shot delay: `initiate_capture` fires
immediately after the drain. The countdown + black/red "waiting for camera"
screen observed in Benro Connect is therefore **app/firmware-side** (polestar_app
or Benro Connect), not the libgphoto2 wait loop. The Bulb timer value being used
as a pre-shot pause (#120) lives in that layer, not here. This trace confirms the
libgphoto2 side does NOT insert any pre-shot delay — so #120's fix belongs in the
firmware/app, and the libgphoto2 exposure budget (phase 3) is the only place the
Bulb duration is consumed.

### C3. Exposure-phase vs total-budget split is correct but only for attribution

`pentax_exposure_phase_ms()` budgets only the exposure (bulb seconds or
multi-shot x base), excluding post-exposure processing margin. This is used ONLY
to attribute a timeout to the right phase in the error message (issue #111). The
actual wait loop uses `capture_timeout_ms` (the total). So the #111 fix improves
diagnostics, not the wait itself — a Bulb capture that times out still times out;
it just says which phase failed. This is consistent with "Bulb shooting does not
work at all" persisting on o-v9r: the budget is now correct, but if the camera
never publishes a candidate within the exposure phase (e.g. the Bulb timer value
is being consumed as a pre-shot pause by the firmware, #120), the exposure-phase
timeout fires.

### C4. Pre-capture drain + post-capture reconcile both bounded at 60/120 s — cascading latency

Worst case between two shots in astro pixel-shift RAW+JPEG:
- pre-capture drain: up to 60 s (8 candidates) + 10 s idle wait = 70 s
- exposure: up to 270 s (4 x 60 s + margin)
- reconcile: up to 120 s
- post-capture idle: up to 60 s (C1)
Total ~520 s of potential latency before the next shutter can fire cleanly. Each
bound is individually reasonable, but they stack. A single "capture interval"
budget that accounts for the mode would let the app show an accurate countdown
and avoid firing the next release mid-processing.

### C5. The session re-init loop (#119) is orthogonal but compounds everything

The dlopen + 64-shim re-registration loop means each capture attempt may start
from a freshly-initialized session, re-running the full pre-flight (phase 0-1).
If the camera is mid-processing from the previous shot, the fresh session's
pre-capture drain sees the pending candidate and spends up to 70 s clearing it
before the new exposure can fire. Fixing #119 (hold the session open) removes one
full pre-flight per frame and is a precondition for the C1/C4 latency fixes to
matter in practice.

## Ownership summary

| Symptom | Layer | Issue |
|---|---|---|
| 1-min stall between astro/multi-shot shots | libgphoto2 (C1: mode-blind 60 s idle wait) | #112, #73 |
| Countdown = pre-shot pause, not exposure | firmware/app (C2) | #120 |
| Bulb shot "failed" report after file written | libgphoto2 diagnostics only (C3) | #111 |
| Cascading 60/120 s bounds stack up to ~520 s | libgphoto2 (C4) | #73, #112 |
| Fresh session per frame re-runs pre-flight | stage-2 loader (C5) | #119 |

## Acceptance for the next wave

1. Make `IDLE_WAIT_MAX_MS` mode-aware (reuse `pentax_capture_timeout_ms` sizing).
2. Add a unit test: astro pixel-shift RAW+JPEG post-capture idle wait budget > 60 s.
3. Confirm #120's pre-shot pause is in polestar_app/Benro Connect, not libgphoto2
   (this trace proves the libgphoto2 trigger fires immediately after drain).
4. Fix #119 (hold session open) so the pre-flight does not re-run per frame.
5. Re-test Bulb on o-v9r with the camera attached: verify a >60 s Bulb publishes
   a candidate within `exposure_phase_ms` and the post-capture idle wait completes
   without the 60 s cap firing.
