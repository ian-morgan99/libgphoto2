# K-3 III capture-mode diagnostics — ownership findings

Revision: 2026-09-18

Derived protocol facts for the K-3 III (`25fb:0189`) capture-mode defects
reported on Polaris `o-v9q-bulb-timeout` (build_id `6.0.0.54.8`, libgphoto2
`dacfc8986`). These are ownership findings from the decompiled IMAGE
Transmitter 2 client and the Benro Connect command map; they are not a claim
that the protocol has passed hardware validation.

## #112 — Astro Bulb 3-minute cap and intervalometer first-shot delay

### 180 s Astro cap: camera-reported, not an app-side UI cap

The 3-minute (180 s) cap on the Astro-mode Bulb time control is **camera-
reported**, owned by the camera's `AstroTracerTimeLimit` value, not a
hard-coded app-side UI limit.

Evidence from the decompiled IMAGE Transmitter 2 client:

- `MtpDevice.AstroTracerTimeLimit` (property RID 175) returns the field
  `_camAstroTracerLimitTime`.
- That field is populated from the camera's `GetAllConditions` response at
  byte offset **528** (`BitConverter.ToUInt32(array2, 528)`), guarded by
  `array2.Length > 524`. This is the same conditions payload libgphoto2
  parses; libgphoto2 reads the identical field into
  `PentaxConditions.astro_limit_seconds` (offset 528) and sets
  `has_astro_limit = 1` when the payload is long enough.
- The client clamps the app Bulb timer to that camera value **only** in
  Astro exposure mode: in `MainWindow.xaml.cs`, when
  `ExposureMode == 20` (Astro) and capture was not initiated from the app,
  it computes `TimeSpan(0,0,0,(int)AstroTracerTimeLimit)` and reduces
  `TSBulbTime` to that value if it is larger. Manual Bulb (non-Astro) is not
  clamped by this path.

**Ownership conclusion:** the 180 s cap is the camera's
`AstroTracerTimeLimit` (conditions offset 528). Preserve it for Astro mode;
do not copy it into Manual Bulb. libgphoto2 already surfaces the same value
via `PentaxConditions.astro_limit_seconds` / `has_astro_limit`, and
`pentax_capture_timeout_ms()` honours it on the
`PENTAX_CONDITION_ASTRO_SHIFT_MODE` path.

### First-shot delay: Polaris workflow, not libgphoto2 capture timing

The intervalometer applying its full period before shot #1 is a **Polaris
workflow/scheduler** concern, not a libgphoto2 capture-timing defect.

- The Benro Connect command map identifies **272 = `SP_DELAY_SHOT`** and
  **306/307 = GET/SET_INTERVAL_TYPE**. The first-shot delay is driven by the
  interval-type / delay-shot state, not by libgphoto2 capture timing.
- The IT2 client's app Bulb timer starts the exposure immediately; its only
  pre-exposure wait is explicit self-timer handling (DriveModeNew 7 → 12 s,
  8 → 2 s).

**Ownership conclusion:** interval-only shot #1 should fire immediately
unless an explicit Delay Shot / self-timer is selected. Trace command 272
plus the interval-type state (306/307) when starting Astro interval capture
and distinguish *initial delay*, *exposure duration*, and *inter-shot
interval*. Fix the scheduler/command interpretation in the Polaris runtime;
do not compensate via camera timeout changes.

## #113 — Bulb duration does not stick across shots or mode switches

### Ownership: two distinct Bulb-timing mechanisms

The decompiled IT2 client separates **two** Bulb-timing mechanisms, and the
"duration does not stick" symptom must be attributed to the correct one
before editing the shutter setter:

1. **Camera Bulb timer** — `IsCamBulbTimer` / `CamBulbTimerValue`. This is
   camera state (the camera's own Bulb timer value).
2. **Application Bulb timer** — `TSBulbTime`, stored in
   `Settings.Default.DefaultBulbTime` by `StartBulbTimer()` and restored by
   `StopBulbTimer()`. IT2 uses it to issue the second `CamRelease()` edge.

Because "Bulb duration" is not necessarily a shutter-speed property, the
`_put_Ricoh_ShutterSpeed` write path (with its 5-attempt verify loop that
re-reads conditions) should **not** be treated as the leading suspect until
logs prove Benro maps the UI value there.

### Commands to trace

Record, around *enter Bulb → set duration → shoot → return → mode away/back*:

- **261 = `SP_SET_SHUTTER`**
- **268 = `SP_GET_SHUTTER_INFO`**
- **298 / 299 = `SP_GET_EX_TIME` / `SP_SET_EX_TIME`**
- the resulting Pentax conditions (camera Bulb timer value)

### Ownership decision rule

Produce one evidenced line before editing:

```
UI duration -> command X -> [Polaris state | camera timer property | shutter property]
```

- If **298/299** own the displayed duration, fix their handler/storage.
- If the **camera Bulb timer** owns it, preserve camera truth (do not
  synthesize a shutter value).
- If it is **app-only** state, do not invent a PTP property.
- Ensure the Astro `AstroTracerTimeLimit` clamp (see #112) is **not** copied
  into Manual Bulb; otherwise the Astro clamp can be misdiagnosed as a
  persistence failure.

## #115 — Panorama and related modes: capture fails

### Panorama is a dedicated firmware workflow command, not ordinary capture

The decompiled Benro Connect command map proves **271 = `SP_PANORAMIC`** is a
dedicated firmware workflow command, alongside:

- **270 = `SP_FOCUS_STACK`**
- **272 = `SP_DELAY_SHOT`**
- **280 = `SP_HDR`**

rather than ordinary command 264 photo capture. Panorama should therefore
**not** initially be modelled as "normal capture plus multi-candidate
reconciliation".

Note: historical OpenPolaris camera command mappings were badly mislabelled;
command 271 had been called `CAM_SET_IMG_SIZE` despite Benro ground truth
being `SP_PANORAMIC`. For Benro Connect testing, use the decompiled PolarisCMD
mapping as protocol truth.

### Diagnostic boundary: stay above libgphoto2 until the trace crosses in

For one failing repro, correlate Mlog/Clog with the socket command stream and
answer **first**: is 271 received, parsed, and does the Panorama workflow
start? Then follow each workflow-triggered camera capture into
pgphoto/libgphoto2. Only if 271 successfully starts and a child camera
capture fails should #111 / reconcile / session logic become the leading
suspect.

Record, at each boundary (command parser → Panorama workflow → child request
→ pgphoto → libgphoto2 → candidate/transfer → workflow completion), the
**first** abnormal return. Stop diagnosis at the first failing boundary.

### Scope table: compare Focus Stack and HDR

Repeat minimally for 270 (Focus Stack) and 280 (HDR); make a 3-row table:

| Command | Received? | Handler starts? | Child capture starts? | Child succeeds? | Workflow completes? |
|---|---|---|---|---|---|
| 271 Panorama | | | | | |
| 270 Focus Stack | | | | | |
| 280 HDR | | | | | |

Interpretation:

- All fail **before** child capture → shared Polaris workflow.
- All enter pgphoto and fail identically → common camera bridge.
- Panorama only → Panorama-specific logic. Do not enlarge reconcile bounds
  merely because it is multi-shot.

Verify ordinary command-264 capture, each affected workflow, and preview/
session state after completion/cancellation.
