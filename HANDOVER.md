# Pentax libgphoto2 Fork — Session Handover Document

**Date:** 2026-08-22 (updated 2026-08-25)
**Workspace:** `/home/ian/Documents/VSCodeProjects/LibGphoto2` (+ sibling `BenroPolarisPatcher`)
**Purpose:** Complete handover for continuing Pentax tethering development. Read this fully before acting.

> **2026-08-25 update:** A step-by-step hardware-testing runbook now exists at
> `libgphoto2/docs/pentax/HARDWARE_TESTING_RUNBOOK.md`. It supersedes §2 below
> in level of detail (staged-IOLIBS workaround, GVFS kill procedure, USB-mode
> blocker, wedge recovery, branch policy). Read the runbook FIRST if your task
> involves real cameras. All 2026-08-24 work is committed and pushed to
> `origin/master`; `master` is the only integration branch.

---

## 1. PROJECT GOVERNING RULES (non-negotiable)

1. **Image Transmitter 2 (IT2) decompiled source is the normative protocol reference** (`ImageTransmitter2/IMAGETransmitter2/MtpDevice.cs`, `MainWindow.xaml.cs`, `Defines.cs`, LUT files). Before implementing ANY feature, trace IT2's complete call path first.
2. **PTP acknowledgement ≠ application.** Every write must be verified through live conditions polling (IT2's own oracle), never descriptor read-back alone, never the CLI exit code.
3. **Fail-closed always.** Unknown values stay numeric ("unknown-N"). Never offer widget choices beyond what the camera's descriptor advertises. Never guess constants.
4. **One variable per hardware test**, with mandatory exact restoration, verified independently in a fresh session.
5. **Never without explicit operator approval:** shutter release, focus drive, live-view start, astrotracer activation, deletes, resets.
6. **Default builds must NOT expose capture/preview.** Research capture only via compile-time `-DLIBGPHOTO2_ENABLE_PENTAX_RESEARCH_CAPTURE=1`.
7. **Every hardware test gets recorded** in `libgphoto2/docs/pentax/REAL_HARDWARE_TEST_LOG.md` (chronological table) and evidence under `docs/pentax/evidence/<date>/`.
8. **LESSON LEARNED (2026-08-22):** Check IT2's treatment BEFORE concluding anything about model differences. The "K-3 III different drive wire format" theory was wrong — IT2 parses 0xd013 as BYTE enum from descriptor for ALL models; differences come from capability flags + exposure-mode UI visibility.

## 2. BUILD & RUN ENVIRONMENT

> **Superseded in detail by `libgphoto2/docs/pentax/HARDWARE_TESTING_RUNBOOK.md`** —
> consult it for the staged-IOLIBS workaround (probe harnesses), the
> `pentax_research_capture` meson option, GVFS kill procedure, and USB-mode
> checks. Summary retained below.

```bash
# One-time deps (already installed):
sudo apt-get install -y libgphoto2-dev gphoto2 libxml2-dev libusb-1.0-0-dev libexif-dev
uv tool install meson ninja   # ~/.local/bin

# Build (research capture ENABLED in current _build):
cd /home/ian/Documents/VSCodeProjects/LibGphoto2/libgphoto2
export PATH=$HOME/.local/bin:$PATH
meson setup _build --buildtype=debugoptimized \
  -Dcamlibs=ptp2,pentax,directory -Diolibs=disk,libusb1 \
  -Dc_args="-DLIBGPHOTO2_ENABLE_PENTAX_RESEARCH_CAPTURE=1"
ninja -C _build

# CRITICAL workaround: ltdl can't find camlibs in meson subdirs:
ln -sf _build/camlibs/ptp2/ptp2.so _build/camlibs/ptp2.so
ln -sf _build/camlibs/pentax/pentax.so _build/camlibs/pentax.so
ln -sf _build/camlibs/directory/directory.so _build/camlibs/directory.so

# Run environment for every gphoto2/harness invocation:
B=$PWD/_build
export LD_LIBRARY_PATH=$B/libgphoto2:$B/libgphoto2_port/libgphoto2_port
export CAMLIBS=$B/camlibs IOLIBS=$B/libgphoto2_port/libusb1
```

**USB ownership:** GVFS daemons (`gvfsd-gphoto2`, `gvfsd-mtp`) claim cameras on plug-in and RESPAWN. Kill by PID found via `/proc/*/fd` scan (fuser sometimes misses them):
```bash
for p in $(pgrep -f "gvfs|nautilus"); do ls -la /proc/$p/fd 2>/dev/null | grep -q "bus/usb" && kill -9 $p; done
```
Note: sandbox terminals intermittently show "Read-only file system" for /tmp — retry or use workspace paths.

**Entangle (GUI tethered shooting):** launch with `LibGphoto2/run-entangle.sh` — it kills GVFS USB holders, exports the fork env, and execs Entangle. Verified: both bodies advertise Image+Preview capture; all Pentax widgets (incl. focus peaking, PC-LV, bracketing, CI mode, movie mode) appear under /main/status/.

## 3. HARDWARE IDENTITIES

| Camera | PC-P/MTP PID | MSC PID | Model No | Status |
|---|---|---|---|---|
| K-3 Mark III | `25fb:0189` | ? | 78420 | Primary body, fw 2.20 |
| K-3 Mark III Monochrome | `25fb:018f` | ? | (same family) | In ptp2 USB table **and** in pentax_lookup_model (StartsWith match, shares K-3 III flags) — vendor mode engages on string match. Hardware-untested; extrapolated from colour K-3 III. |
| K-1 Mark II | `25fb:0183` | `25fb:0182` | 78400 | Second body, fw 1.02. 0182 must NEVER go in ptp2 (storage-only) |
| K-1 | `25fb:0179` (PTP) | ? | 77970 | In ptp2 table, not vendor-enabled |

Port mapping changes on reconnect — always re-derive from sysfs:
```bash
grep -H . /sys/bus/usb/devices/*/idVendor /sys/bus/usb/devices/*/devnum | paste - - | grep 25fb
```

## 4. WHAT'S IMPLEMENTED (all in camlibs/ptp2/)

### Widgets under /main/status/ (vendor-gated: supported_model && vendor_mode_enabled)
| Widget | Property | Write? | Verification |
|---|---|---|---|
| pentaxconditions | 0x900f read | RO | — |
| pentaxliveviewcontrols | d009/d036/d037 reads | RO | — |
| pentaxpropXXXX (10 widgets) | raw descriptors | RO | — |
| pentaxdirectshutter | d00f UINT64 num/den | ✅ | live conditions bulb-seconds, 5×100ms polls |
| pentaxdirectiso | d01e UINT32 | ✅ | live conditions ISO field |
| pentaxdirectaperture | 5007 UINT16 hundredths | ✅ | conditions aperture num/10 |
| pentaxdirectev | 5010 INT16 thousandths | ✅ | conditions tenths (thousandths/100) |
| pentaxdrivemode | d013 | ✅ | conditions drive field; **choices gated by camera descriptor enum** (IT2-faithful fix) |
| pentaxdirectwb | 5005 UINT16 | ✅ | descriptor re-read; choices from IT2 `_camToMtpWBTable` + `auto-800f` for K-3 III |
| pentaxliveviewzoom | d037 12-byte compound | ✅ | read-back; single 16x→10x fallback on 0x201c only |
| pentaxliveviewafposition | d036 8-byte compound | ✅ | read-back; geometry bounds check; **K-1 II HW-verified** (bounds exclusive; K-1 II never echoes — always reports centre 360,240) |
| pentaxfocuspeaking | d02b UINT8 (0/1/2) | ✅ | read-back verify; K-1 II + K-3 III both HW-verified |
| pentaxpclvmode | d035 UINT8 (0/1) | ✅ | no read-back (K-3 III reflects only after LV restart); K-3 III HW-verified |
| pentaxbracketmode | d014 UINT8 | ✅ | off/3/5/+2/-2 (IT2 nominal idx); K-3 III HW-verified |
| pentaxbracketstep | d015 UINT8 | ✅ | descriptor enum, display /10; K-3 III HW-verified |
| pentaxcimode | d020 UINT8 | ✅ | 14 modes via CIModeLUT codes; K-3 III + K-1 II HW-verified |
| pentaxcompositionadjust | d02a UINT8 | ✅ | on/off; set+get same session (lazy read-back); K-3 III HW-verified |
| pentaxcrossprocess | d02c UINT8 | ⚠️ | K-3 III: writes work once CI mode set to cross process. K-1 II (2026-09-02 probe): GET 0x200a / SET 0x200f even in that mode → gated k3iii-family only (`pentax_model_supports_cross_process`), fails closed on other models |
| pentaxmoviemode | d039 UINT8 | ✅ | on/off; K-3 III HW-verified |
| manualfocusdrivenear/far | 0x9017 | K-3 III only | one-shot, no escalation |

### Shared helpers (config.c) — REUSE THESE for new writes
- `_pentax_exposure_write_preflight(params, change_bit, &conditions)` — double conditions sample 100ms apart, checks changeability bit + idle/not-task-changing
- `_pentax_verify_rational_in_conditions(params, num, den, get_num_fn, get_den_fn)` — 5×100ms bounded verification
- `ptp_pentax_set_device_prop_raw()` (ptp.c) — raw 0x1016 sender for compound payloads

### Capture path (library.c)
`camera_pentax_capture`: 0x9011 initiate → conditions offset 32 candidate poll → 0x900b metadata → 0x900c/d block transfer → 0x900e finalize. State machine in params->pentax.transfer_state.

### Key source anchors (IT2)
- Model capability flags: MtpDevice.cs `Model` setter (~line 84–200). K-3 III sets many flags incl. `_isSelfTimerContinuousNewSupported`; K-1 II sets almost none
- Drive list: `RefreshDriveModeList()` — BYTE enum from descriptor, ALL models
- WB wire table: `_camToMtpWBTable` (~line 6364); K-3 III Auto = 0x800f (not in table, we added "auto-800f")
- EV list: `RefreshExpCompansationList()` — INT16 thousandths from descriptor enum
- Shutter: `TvCurrentIndex` setter → UINT64 LE of `_TvList[index]`; Bulb domain when conditions bit 0x40 set
- AF position payload: `{2,0,0,0,Xlo,Xhi,Ylo,Yhi}`; zoom: `{4,0,0,0,Xlo,Xhi,Ylo,Yhi,mag,0,0,0}`
- Focus: K-3 III new-focus 0x9017 displacement `(int)(-(openAvNum*2.5/3.0)*dir)` from conditions offset 328; K-1 II old-focus 0x9016 (amount, direction) — **RESOLVED 2026-08-24: have_prop gate relaxed for old-focus models; `oldfocusdrivenear`/`oldfocusdrivefar` both returned 0x2001 OK mid-LV with no wedge; camera healthy after. The AF-mode precondition in _put_Pentax_OldFocusDrive remains the safety guard against the MF wedge.**

## 5. HARDWARE EVIDENCE STATUS (as of 2026-08-24)

### PROVEN (HW-W, restored & verified)
On **K-1 II**: ISO 200→400→200; shutter 1/500→1/125→1/500; aperture f/2.0→f/2.2→f/2.0; EV 0→+0.3→0; drive single→cont-lo→single; WB auto→daylight→auto; LV zoom off→2x→4x→off; **focus peaking off→on roundtrip (first successful d02b write on this body)**; **still capture+transfer+finalize (K1II8665.DNG)**.
On **K-3 III**: ISO 3200→1600→3200; aperture f/3.5→f/4→f/3.5; **Bulb timer 300s→600s→300s (reproducible)**; WB auto-800f→daylight→auto-800f; drive single→mirror-up→single (descriptor-gated); focus Near/Far qualitative (earlier session); EV 0→+0.3→0; **focus peaking off/on/on+outline roundtrip; PC-LV on/off**.

### KNOWN GAPS / FAILURES
- **EV on K-3 III**: RESOLVED — advertisement is mode-dependent; in current mode 31 enums appear and writes work (0→+0.3→0 verified).
- **Drive modes beyond descriptor enum are silently ignored** — now correctly gated.
- **AF-position WRITE**: TESTED on K-1 II 2026-08-24 — works; bounds check exclusive (720,480 rejected); K-1 II never echoes selection (always centre 360,240), handled by special-case at config.c:10252.
- **K-1 II focus 0x9016**: RESOLVED 2026-08-24. Root cause of the earlier wedge was twofold: (a) the widget gate culled the focus widgets because the K-1 II never advertises 0x9016 in DeviceInfo, and (b) the original attempt ran with MF selected. have_prop now admits PTP_OC_PENTAX_FocusControl on old-focus-family models with vendor mode enabled; _put_Pentax_OldFocusDrive enforces the AF-mode precondition. Hardware verified: `oldfocusdrivenear` and `oldfocusdrivefar` both return 0x2001 OK mid-LV with no wedge; camera healthy after (battery 100%, responsive).
- Bulb OPEN-SHUTTER exposure (actual long exposure start/stop) NOT tested — only timer setting writes. Tier 11. Probe harness ready: `tests/bulb_probe.c` takes `<hold_ms> <port> <release_mode>`; release_mode=2 is the bulb-open candidate per K-3 III traces. Run env: IOLIBS must point at staged copies of the built port drivers (`cp _build/libgphoto2_port/libusb1/usb1.so /tmp/iolibs/libusb1.so`, same for usbscsi) and LD_LIBRARY_PATH must include `_build/libgphoto2_port/libgphoto2_port` alongside `_build/libgphoto2`.
- **Capture wait budget now duration-aware** (2026-08-24, library.c): `pentax_capture_timeout_ms()` sizes the candidate poll from live conditions — astrotracer shift mode extends to astro_limit_seconds + 30 s processing margin; a set bulb timer extends to bulb_timer_seconds + 30 s; floor remains 60 s. Addresses pixel-shift/astro captures taking ≥4× base exposure plus processing.
- Astrotracer: status flags read-only; no activation path implemented.
- Pixel Shift: only a status flag (conditions offset 320 bit 10); IT2 drives it via drive mode + capture, no direct property.
- Movie mode (d039), bracketing mode/step (d014/d015), CI mode (d020), composition adjust (d02a): now dedicated widgets, HW-verified on K-3 III (see widget table). Cross process d02c SET parked. On K-1 II these five return empty data phase (-102) on GET; model gating added 2026-08-24 returns GP_ERROR_NOT_SUPPORTED instead — **regression PASSED (evidence_k1ii_test.txt: all 5 fail closed with -6 before any PTP I/O, no -102 seen)**.
- **Silent-ignore on LV-only props**: compositionadjust (d02a) and focuspeaking (d02b) SETs return PTP OK but are discarded when Live View is inactive (confirmed on K-3 III 2026-08-24; matches IT2 UI which only sets them from PcLiveViewStart context). LV-active retest attempted same day: PC-LV cannot be held open via pentaxpclvmode alone (d035 SET → 0x2001); the only working LV window is inside `--capture-preview`, which restores d035=0 on exit and the camera resets d02a/d02b to 0 at LV stop. Persistent-LV mode required before these widgets can be verified end-to-end.
  - **Wire-level diagnostic rule (2026-08-24, /tmp/bm.log + cap.log)**: a silent-ignore SET shows OK after ~120 ms with ZERO 0x400c interrupt events; a real state change (e.g. d035=1) shows OK after ~430 ms WITH 0x400c events. Use IRQ presence during the set window to distinguish accept-vs-ignore without a read-back.
  - **Persistent LV across processes is architecturally impossible** — d035=1 alone does not sustain PC-LV; continuous 0x9006 polling does (IT2 polls at 33 ms; one frame ≈75 KB in ~16 ms). End-to-end d02a/d02b verification must happen inside ONE session that loops 0x9006 (extend capture-preview or add a research harness).
  - **RESOLVED (2026-08-24 cont. 2)**: research harness shipped as session-local config toggle `pentaxpclvkeep` — when set, `pentax_restore_live_view()` skips the d035 restore, so PC-LV persists across `--capture-preview` calls in one process. Results: **d02b focuspeaking write ACCEPTED and sticks mid-LV** (OK ~66 ms, read-back on+outline, survives a second preview frame); **d02a compositionadjust STILL silently ignored even during active LV** (OK ~7 ms, no IRQ, read-back off — likely needs the full composition-adjust capture flow); **d039 moviemode sticks mid-session** but reverts to off when the vendor session closes, confirming session-scoping.
  - **Bracketmode d014 is also silently ignored on K-3 III** (both values 01/03/05 tried; wire signature identical to d02a/d02b). Moviemode d039 IS actively processed (one 0x400c event) but is session-scoped: resets to off when the vendor session closes.
- Advertised-but-rejected on K-3 III in M mode (0x201c): 5014/5015 contrast/saturation, d021–d029 booleans set=1. d018 color temp GET fails; d01b/d02d/d036/d037 TEXT/compound writes rejected (0x201d/0x2002).
- **Card writing mode 0x9004 + Green button/interrupt 0x9013: implemented (2026-08-27), committed in the fork as `aebcb84ad`. Both HW-verified on live K-1 II (2026-09-01): Green-button probe run → raw PTP RC 0x2001; card-write re-run after power-cycle + cable swap → all four modes off/SD1/SD2/both return 0x2001 OK, state restored to off(0) (earlier same-day runs had returned universal DeviceBusy 0x2019 — clearing factor: fresh session vs SD card in SD2, not yet isolated).**
  - 0x9004: `ptp_pentax_set_card_writing_mode()` wrapper (ptp.c) + config widget `pentaxcardwritingmode` (radio off/SD1/SD2/both → bitmask 0–3, IT2 MtpSetCardWritingMode signature). Gated to dual-slot models per IT2 `_isDualSlot` (K-1 II/K-1/K-3/K-3 III family/645D/645Z; KP and K-70 are single-slot in IT2 — capability-matrix line 353 was imprecise). No GET opcode exists, so the get handler reports a session-local cache (`sd1/sd2_writing_mode` in the pentax struct, zeroed on identify) that is updated only after a successful set; on failure the cache keeps its last values (IT2 fail-closed restore semantics, MtpDevice.cs SDWritingMode setter).
  - 0x9013: wired into `camera_trigger_capture` as a PENTAX branch — research builds (`LIBGPHOTO2_ENABLE_PENTAX_RESEARCH_CAPTURE`) call `ptp_pentax_interrupt()` (IT2 GreenButton, no data phase); default builds log and fall through to the generic InitiateCapture path. NOT used in the abort path (cancellation semantics unverified; TerminateCapture 0x9012 remains the verified cancel).

### Baseline states (restore targets)
- K-1 II: M mode, ISO 200, 1/500, f/2.0, EV 0, WB auto, drive single
- K-3 III: mode raw 20, ISO 3200, bulb-timer=yes 300s, f/3.5, EV 0, WB 0x800f, drive single

## 6. MODEL SUPPORT — WHO'S IN, WHO'S OUT

### Vendor-mode enabled in fork (pentax_lookup_model, pentax-utils.c): K-3 III (0189), K-1 II (0183), K-3 III Monochrome (018f→78420), KP (017f), K-70 (017d), 645D (0130, ext ver 0), K-3 (0165), K-1 (0179), GR III (210f). **K-3 II (017b) is NOT in IT2 → fail-closed, vendor mode stays off** (operator ruling: assume older architecture until proven by hardware or a newer IT2 build).

### 2026-08-22 model additions (code-only, build clean):
- All IT2 models now covered EXCEPT 645Z (no PTP PID in ptp2 table; IT2 IsSupported gates it on fw ≥1.23 — add once PID confirmed) and G900SE (LS-family, not a candidate).
- `pentax_model_uses_new_focus()` = K-3 III family + KP + GR III + K-3 II (per IT2 Model setter flags).
- 645D: IT2 matches bare string "645D" (manufacturer field "PENTAX", not RICOH) — lookup accepts both "645D" and "PENTAX 645D".
- **HW finding:** K-01 PTP PID is `25fb:0131` (not 0x0130 as previously assumed; 0x0130 = 645D/K-01 legacy). K-01 reports model string "K-01", gets 0 pentax widgets (fail-closed correct), vendor mode NOT engaged.
- **HW sanity check (2026-08-22, ports usb:001,007=K-3 III / usb:001,008=K-01):** K-3 III vendor enable OK (function flags 0x0), 19 pentax widgets; K-01 0 pentax widgets.

### WiFi Commander for Pentax v1.76 analysis (wificommanderpentax_v176/):
- Windows .NET app (ConfuserEx-protected, decompiled via strings mining; extracted under wificommanderpentax_v176/_extracted/). Talks to camera over the camera's built-in Wi-Fi HTTP API (Pentax "Remote" protocol), NOT USB PTP. Commands: camera_shoot, camera_shoot_bulb, camera_shoot_movie, camera_self_shoot, camera_shoot_planned*, set_selftimer, drive_single/continuous, xfocus/yfocus, zoomlvl, shot_interval, timerexposure, bulbmode.
- **Astrotracer mode 3:** enum EMASTRO_TRACER / _B / _BT / _TYPE3 + `astroTrackingTime` property + `InAstroTracerType3Mode`. IT2 only knows modes 1/2 (B=bulb, BT=bulb+timer). Mode 3 is a newer firmware astrotracer mode exposed via the Wi-Fi API. For USB: likely a new value in the drive-mode/astro property space — candidate for investigation once we have a body with fw supporting it; do NOT guess the wire constant (fail-closed rule).
- Verdict: transport differs from our USB path, but mode 3 + astroTrackingTime are worth adding to the capability matrix as "Wi-Fi-only evidence" rows.

### IT2 version in use: decompiled reference is **v2.6.1.3** (AssemblyInfo.cs). K-3 III needs ≥2.6.0 ✓, KP ≥2.3.0 ✓. Latest-IT2 download check was blocked by network (DNS) — retry when online.

## 8. IMMEDIATE NEXT STEPS (priority order)

1. ~~Add K-3 III Monochrome + KP + K-70 to pentax_lookup_model()~~ **DONE 2026-08-22** — plus K-3, K-1, GR III, 645D, K-3 II (see §6 additions). Build clean.
