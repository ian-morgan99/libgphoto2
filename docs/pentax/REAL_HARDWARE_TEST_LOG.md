# Pentax real-hardware test log

This is the canonical, chronological record of tests executed against a
physical Pentax camera. It is separate from `DEVELOPMENT_PLAN.md`, which tracks
work and acceptance gates, and from the smaller per-gate evidence records.

Camera: PENTAX K-3 Mark III, firmware 2.20. Test date: 2026-08-21. The camera
serial number and all image content are deliberately omitted. Unless explicitly
stated otherwise, tests used the colour K-3 Mark III in MTP mode over USB.
The K-1 Mark II H1.7 rows explicitly identify the second body and used its PC-P
USB mode; its firmware version is 1.02.

## Non-destructive test policy

Every hardware test in this record was bounded and selected to avoid persistent
camera or card changes. Before direct USB access, GVFS was unmounted and the USB
device node was checked with `fuser`; a probe was aborted if another process
owned the device. Candidate builds ran from a read-only source mount in a
disposable Ubuntu container.

No completed test in this record issued a still-capture, shutter-release,
delete, file-write, reset, or firmware-update command. One explicitly approved
ISO write changed 3200 to 1600; battery loss interrupted its restore, and that
open restoration obligation is recorded below.
Preview JPEGs existed only in memory or disposable storage and were not retained.
Camera serial data seen during discovery was not committed.

## Chronological results

| Test | Operations allowed | Result | Code consequence |
|---|---|---|---|
| Passive enumeration in MTP mode | Linux USB descriptor reads only | `25fb:0189`; Imaging interface; 480 Mbit/s | Replaced speculative K-3 III PID `018c` with observed PID `0189` |
| Passive enumeration in CD-ROM mode | Linux USB descriptor reads only | `25fb:018a`; Mass Storage interface | Documented as installer mode, not the tethering transport |
| Generic MTP discovery | Forced generic PTP/MTP DeviceInfo and storage reads | Exact model `PENTAX K-3 Mark III`; firmware 2.20; Microsoft MTP extension `0x00000006`; roots SD1 and SD2 | Removed the assumption that Pentax routing could depend on a Pentax VendorExtensionID |
| Pre-vendor descriptor baseline | DeviceInfo/property reads | Only the standard MTP operation/property set was visible | Established a baseline for checking vendor-mode teardown |
| Guarded vendor-mode lifecycle | Pentax `SetVendorMode(enable)`, descriptor reads, then `SetVendorMode(disable)` | Enable exposed Pentax descriptors; a separate generic session after exit showed the original five properties | Routed the session using exact VID, PID, and DeviceInfo model; added post-enable DeviceInfo refresh and rollback |
| Read-only property descriptors | Descriptor/value reads only | WB `0x5005`, F-number `0x5007`, exposure compensation `0x5010`, shutter `0xd00f`, and USB live view `0xd035` were present; standard ISO `0x500f` and focus mode `0x500a` were absent | Unsupported properties remain withheld; unknown `0xd01e` semantics were not invented |
| Read-only configuration widgets | Candidate configuration reads only | WB, aperture, exposure compensation, and shutter widgets decoded; unmapped WB values remained explicitly unknown | Corrected effective-vendor routing for Pentax configuration tables |
| Single preview frame | Temporary live-view enable, one `0x9006` frame read, restore original live-view value, vendor-mode disable | Complete baseline JPEG, 1080x720, 84,488 bytes; independent post-check reported `0xd035=0` | Validated the first live-view vertical slice |
| Initial unpaced preview loop | Same temporary live-view operations; no retention | Frames 1 and 2 were valid; frame 3 returned an error; cleanup ran | Added conservative inter-frame pacing and precise failure-stage reporting |
| Paced preview loop | Ten in-memory previews at 250 ms spacing | 10/10 complete JPEGs; explicit camera exit succeeded | Established a safe repeatable harness |
| Paced preview soak after cleanup hardening | Fifty in-memory previews at 250 ms spacing | 50/50 complete JPEGs; strengthened camera exit reported cleanup success | Preserved live-view restoration errors instead of allowing later cleanup success to mask them |
| First cold camera restart and read-only config enumeration | Two full config-tree reads, followed by one named shutter read with sanitized lifecycle logging | Camera re-enumerated as `25fb:0189`, but vendor enable `0x9001` returned General Error `0x2002`; only the five generic MTP properties were exposed and no later Pentax opcode was sent | Treat vendor entry as state-dependent and fail closed; do not claim restart stability or vendor settings in this session |
| Delayed cold-session retry | Release USB ownership, wait five seconds, open a fresh session, and request the named shutter setting | Vendor enable again returned `0x2002`; shutter was absent because the driver remained safely generic | Startup timing is not a sufficient explanation; do not loop retries or continue vendor probes without identifying the missing state/prerequisite |
| Controlled clean power-on lifecycle, cycles 1–3 | Before each cycle confirm camera absent and all prior sessions terminated; user powered on and explicitly reported normal shooting-screen readiness; verify `25fb:0189` and exclusive ownership; make one fresh connection and read shutter descriptor | All three first `0x9001` enables succeeded with flags 0; `0x1014` read `0xd00f`; current shutter was 1 second with 55 enumerated choices from 1/8000 through 30 seconds; all clean exits sent `0x9001` disable and returned success | Initial controlled cold-start checkpoint passes 3/3; contrast with 0/2 less-controlled restart attempts makes camera readiness/UI state the leading hypothesis, not a proven prerequisite |
| Controlled warm reconnects, cycles 1–3 | Leave camera powered on and unchanged; open three independent sessions one second apart; in each, enable once, read only the shutter descriptor, disable once, and exit | All 3 sessions passed; each log contained one successful enable, exactly two `0x9001` requests total, one `0x1014` request for `0xd00f`, and current value 1 second | Initial warm reconnect checkpoint passes 3/3 with no stale vendor state or value drift; larger 10/10 and 50-cycle lifecycle gates remain open |
| Manual-mode full parameter enumeration | Three independent complete public config-tree reads; normalize serial, clock, and battery only | 777 lines per snapshot; normalized snapshots byte-identical; zero `0x1016` writes; decoded WB, aperture, exposure compensation, shutter, sharpness, and raw vendor properties | Establishes stable read-only `M` baseline; `0xd01e` is a strong ISO candidate pending camera-display correlation; see H1.4 |
| First bounded minimum focus probe | Exact K-3 III model and explicit `usb:001,010`; fresh `0x900f`; one intended minimum Near `0x9017`; no retry; Far permitted only after success | Two preliminary runs failed during host-only camera detection and sent no PTP operation. After fixing the missing runtime `libxml2`, the first camera-reaching run returned `GP_ERROR_IO`; cleanup was attempted and Far was not sent. Operator confirmed AF was selected and there was no visible or audible lens movement | INCONCLUSIVE. The initial harness did not distinguish status-read failure from focus-response failure. Do not retry until stage-specific response logging is compiled; see H1.5 |
| Second bounded Near diagnostic | Same one-command/no-retry envelope after compiling raw response messages around `0x900f` and `0x9017` | Returned `GP_ERROR_IO`; neither instrumented setter message appeared; Far was withheld. The artifact was verified to contain the messages, and the operator heard no lens movement | Failure occurred before the instrumented status/focus calls, but the harness did not yet distinguish init from action retrieval. Add host-stage labels before one final diagnostic attempt |
| Final stage-labelled Near diagnostic | Exact model/port, exclusive USB ownership, one intended minimum Near action, host-stage labels, 20-second timeout, no retry or Far | `focus_near=failed stage=camera-init error=I/O problem (-7)` | The harness did not retrieve the focus action or enter its `0x900f`/`0x9017` setter. Close focus testing and return to lifecycle/recovery diagnosis; this result says nothing adverse about the opcode mapping |
| Init-only check after clean camera power cycle | Confirm `25fb:0189` absent and no MTP/test owner; user restarted to normal shooting screen; rediscover as `usb:001,011`; exact-model init-only harness with no config, focus, preview, or capture call | `focus_init=failed stage=camera-init error=I/O problem (-7)` | A clean camera restart alone did not restore this harness's initialization. Instrument vendor-enable/DeviceInfo-refresh substages and raw responses before further hardware work |
| Corrected container init-only check | Combine USB-bus enumeration mount with an explicit Docker device-cgroup grant for only `/dev/bus/usb/001/011`; exact-model init and exit only | Vendor enable succeeded with flags 0; `focus_init=command-accepted cleanup=ok retries=0` | Corrects the preceding diagnosis: bind-mount-only runs could enumerate but not open the device. Camera lifecycle was healthy |
| First correctly isolated minimum Near probe | Same corrected single-device isolation; fresh status, one minimum Near `0x9017`, no retry or Far | Vendor enable succeeded, then host exited 139. Audit found a null `alreadyset` dereference after the transport call; no instrumented status/focus error appeared | `0x900f` and `0x9017` most likely returned OK before the host bookkeeping crash. Null dereference fixed and host tests pass; physical movement observation pending before any further command |
| Operator observation after correctly isolated Near | Visual and audible observation requested before any further command | No visible or audible lens movement | A likely OK transport is not physical acceptance. Log actual status factor, displacement, and raw response using another fixed minimum command before considering Far or any larger value |
| Instrumented fixed minimum Near | Correct dual container isolation; exact model/port; fresh `0x900f`; source-derived one-shot Near; no retry, Far, autofocus, or shutter | Vendor flags 0; `openAvNum=28`; displacement `+23`; `0x9017` response `0x2001`; cleanup OK | Hardware confirms protocol acceptance of nonzero K-3 III new-focus command. Physical movement/direction observation remains pending; no Far yet |
| Instrumented Near physical observation | Operator watched the 24–70 mm lens during the `+23` probe | Focus moved nearer | Minimum Near passes protocol and physical-direction acceptance; one equal Far reversal is now permitted |
| Instrumented fixed minimum Far | Fresh status and one source-derived Far reversal under the same corrected isolation; no retry or follow-up | `openAvNum=28`; displacement `-23`; `0x9017` response `0x2001`; cleanup OK | Far transport passes with exact opposite displacement; physical direction/reversal observation pending |
| Instrumented Far physical observation | Operator watched the 24–70 mm lens after the `-23` reversal | Focus visibly moved farther by about the same small amount; exact equality was difficult to judge | Bounded bidirectional focus gate passes qualitatively. Do not claim calibrated displacement accuracy or enable escalation/larger steps from this result |
| Bulb-mode full parameter enumeration | Physical dial `B`; two complete public config-tree reads under corrected single-device isolation; exact transmitted-opcode audit on snapshot 2 | Both snapshots returned 700 lines. Snapshot 2 sent zero setters, focus, or capture operations. Shutter/`0xd00f` exposed 1–600 seconds, current 300; `0xd013` narrowed from M's 12 choices/current 4 to 3 choices/current 0 | Read-only B baseline passes 2/2. Treat `0xd00f` as a B timer domain; withhold `0xd013` value labels and all writes pending display correlation; see H1.6 |
| First ISO write and interrupted restore | Read `0xd01e=3200`; set 1600; read back; request operator display confirmation; plan exact restore to 3200 | PTP and camera display both confirmed ISO 1600. Before restore, USB node vanished and the restore container did not launch. Camera screen was black; after restart attempt operator identified a flat battery | Property identity/write/read-back passes, but reversibility gate remains open. No restore command was sent; expected retained value is 1600. Charge/replace battery, confirm, then restore 3200 before any further write or exposure |
| H0 exact ISO restoration after charging | Operator confirmed retained ISO 1600; exclusive ownership; fresh read requiring `0xd01e=1600`; one set to advertised 3200; independent fresh-session read; operator display confirmation | All three vendor enables succeeded with flags 0. Pre-read was 1600, the sole write returned success, fresh PTP read was 3200, and the camera display showed ISO 3200. USB node was unowned after exit | Interrupted reversibility gate is closed. `0xd01e` identity and exact 3200→1600→3200 round trip pass; this does not authorize arbitrary ISO values or other setting writes |
| K-1 II passive PC-P identity | USB/udev descriptor reads and ownership check only | `25fb:0183`, Imaging/PTP `06/01/01`, exact USB product string; no node owner | First hardware confirmation of the K-1 II identifier; only ptp2 claims the model |
| K-1 II passive MSC identity | USB descriptor and block-device enumeration only; no SCSI command issued by the test | `25fb:0182`, interface `08/06/50` (Mass Storage/SCSI/Bulk-Only); firmware descriptor 1.02; one 59.5 GiB exFAT card volume mounted by Linux and a zero-capacity second LUN | Confirms a distinct storage-only personality. Serial omitted. Do not add this ID to ptp2; use mounted-volume/Directory Browse support. |
| K-1 II MSC file baseline | Read-only mount/layout enumeration, signature inspection, and SHA-256 of newest-by-mtime file | Two LUNs (one populated, one empty); misleading `K-3 III` volume label; newest file `K1II8664.DNG`, 38,468,076 bytes, SHA-256 `db9ea0c3f402fe6b8d5ad03ca8bcce7d3249ffe073fed7776401d44c1dbdb1a5`; embedded model K-1 Mark II | PASS; no filesystem mutation. Establishes future PTP-vs-MSC comparison artifact and proves labels/filename maxima are unsuitable for discovery. See H1.9. |
| K-1 II LV control-state discovery | Exact-model read-only `0xd009`, `0xd036`, `0xd037`; 40-second timeout; no setter/preview/focus/capture | 720×480 area and active image; CAF 576×320, spot 48×48; 8-byte AF response at centre 360,240; zoom raw 1 | PASS; USB released. Two preceding host diagnostic failures were fail-closed and issued no writes. See H1.10. |
| K-1 II direct descriptor sweep | Nine individual fresh-session descriptor/value reads, complete sequence repeated | Seven conventional descriptors stable; 1/500 s, f/2.0, ISO 200, EV 0, 50 mm, WB raw 2, drive raw 1; `0xd02b` and `0xd035` stable nonstandard payloads | READ-ONLY PASS 2/2; corrects `0xd013` to UINT32; anomalous controls withheld; no writes; USB released. See H1.11. |
| K-1 II single-preview attempt | Exact-model research build, dedicated one-frame in-memory harness, 45-second timeout; operator later reported no card installed | Vendor enable `0x2001`, flags 3; composite preview returned `GP_ERROR_IO`, no JPEG, no retry; error-path exit ran and USB released | INCONCLUSIVE/FAIL-CLOSED. Missing card is a readiness confounder; instrument `0xd009`/`0xd035`/`0x9006`/restore/disable before one inserted-card retry. See H1.12. |
| K-1 II inserted-card preview retest | Exact-model one-frame harness; desktop GVFS claim released first; existing tested build run directly | Vendor enable `0x2001`, flags 3; same composite `GP_ERROR_IO`, no JPEG, no retry; USB released | REPRODUCIBLE FAIL-CLOSED; card absence ruled out as sole cause. Instrument each preview and restoration substage before any further attempt. See H1.12. |
| K-1 II instrumented preview | Exact-model one-frame path with stage responses; card installed | empty `0xd035` pre-read→exact K-1 fallback; start `0x2001`; first frame `0xa008`/0 bytes; stop restoration `0x2001`; USB released | START/RESTORE PASS; frame not ready. IT2 treats `0xa008` as transient via later 33 ms poll. Implement bounded polling before retry. See H1.13. |
| K-1 II bounded single preview | Exact-model research build; only `0xa008` retryable; 30-attempt/1.5 s/cancellation bounds | Start `0x2001`; valid 31,126-byte JPEG on attempt 10 after 312 ms; in-memory validation; cleanup success; USB released | SINGLE-FRAME PASS. Advance to paced 10-frame gate only. See H1.14. |
| K-1 II guarded model read | Exact-model init, vendor enable, read-only camera-model widget, exit | Enable succeeded with function flags `0x00000003`; exact model returned; USB released | Initial K-1 II lifecycle pass; flags differ from K-3 III and remain model-specific evidence |
| K-1 II condition snapshot | Separate exact-model session; one read-only `status/pentaxconditions` widget | 568-byte response parsed: state 0, exposure mode raw 21, drive raw 4, ISO 200, `openAvNum=14`, no active Bulb/Astro/error flags; USB released | Layout is compatible through optional offset 528; no setting semantics beyond reported/raw values are claimed; see H1.7 |
| K-1 II version/display limitation | Separate read-only device-version query; operator observation of camera UI | Firmware 1.02; vendor flags again `0x00000003`; PC-P mode does not show settings on the camera display; USB released | ISO/mode values from PC-P cannot be called visually correlated; use a pre-connect/read/disconnect/post-check procedure later |
| K-1 II manual-reference DNG discovery | Operator disconnected, confirmed ISO 200, took one manual image, then reconnected; enumerate storage/folders/files only | New folder `460_0821` contained sole object `K1II8664.DNG`. An initial highest-sequence heuristic selected an older K-3 III file and was rejected by EXIF | Select reference objects by the operator-created folder/object, never cross-camera filename sequence; no card change occurred |
| K-1 II generic DNG download and EXIF | Standard read-only object download of `K1II8664.DNG`; hash/selected EXIF; delete temporary host copy only | 38,468,076 bytes; SHA-256 `db9ea0c3f402fe6b8d5ad03ca8bcce7d3249ffe073fed7776401d44c1dbdb1a5`; K-1 II, Program AE, ISO 200, 6 s, f/1.4, D FA* 50 mm; card object untouched; USB released | Generic DNG download passes once. ISO 200 agrees across display/status/EXIF; raw mode 21→Program AE remains provisional because readings straddled a disconnect |
| K-1 II B-mode condition and duplicate configuration snapshots | Physical dial `B`; one named condition read followed by two independent complete public configuration reads from the current source build; exact transmitted-opcode audit | All sessions passed with flags `0x00000003` and clean exit. Both configuration trees had 188 lines and normalized identically after clock/battery drift removal. Conditions were 568 bytes: raw mode 9, drive 0, ISO 200, Bulb timer 30/1, inactive Astro/error flags. Each full read used five `0x1014` descriptors plus one `0x900f`; zero forbidden writes/actions | Read-only B baseline passes 2/2. Five advertised descriptors are not a capability boundary: Image Transmitter directly requests additional model-supported codes absent from DeviceInfo. Build its K-1 II source table before guarded direct reads; see H1.8 |

## Pending hardware tests (not executed)

The condition parser and read-only status widget were implemented while the
camera charged. Their fixture/build results are software evidence, not entries
in the chronological hardware table. Now that ISO restoration passed, run H0.1
from the development plan: two B-mode reads, exactly one `0x900f` per named widget read,
response length recorded, no setter/focus/preview/capture/transfer/delete/reset,
and operator correlation limited to ISO and visible Bulb-timer state/value.

The 50-frame result is a bounded soak, not the plan's 500-frame acceptance gate.
Still capture, transfer from camera storage, configuration writes, cancellation,
physical disconnect/reconnect, and the 500-frame gate remain untested on hardware.
One camera power-cycle/re-enumeration has now been observed, but it did not pass
vendor-mode entry and therefore does not satisfy the reconnect gate.

## USB identifier postmortem

### What was wrong

The initial `ptp2` candidate registered the colour K-3 Mark III as
`25fb:018c` and called the transport PTP mode. The attached camera proved that
its tethering setting is MTP and that it enumerates as `25fb:0189`. Its CD-ROM
setting enumerates separately as `25fb:018a`.

### Why it should have been caught immediately

The correct `0189` identifier was not difficult to discover. It was available
from one passive `lsusb` read as soon as the camera was attached. More
importantly, it was already in this repository: upstream commit `1c4f11313`
added K-3 III PID `0189` to `camlibs/pentax/library.c` in 2023. The unfinished
`ptp2` prototype separately contained a commented `018c` entry with no evidence
record. That commented value was promoted by commit `f5eebff6f` before hardware
was available.

The failure was therefore procedural, not a difficult reverse-engineering
problem:

1. The implementation treated a commented prototype value as an input fact.
2. The repository review did not reconcile duplicate model knowledge across
   `camlibs/pentax` and `camlibs/ptp2`.
3. The first plan allowed implementation before the passive USB identity gate.
4. The model test proved internal consistency with `018c`; it did not prove that
   `018c` matched a real device. A synthetic test cannot validate an identifier
   copied into both implementation and expectation.
5. The absent camera was recorded as a blocker, but the unverified identifier
   was still allowed to flow into feature code instead of remaining an explicit
   unknown.

There is no hardware evidence in this work that `018c` identifies this camera
in any mode. It must not be presented as an alternate K-3 III identifier.

### Corrective controls

- Hardware identity is now the tuple `25fb:0189` plus the exact DeviceInfo model
  `PENTAX K-3 Mark III`; VID/PID alone does not authorize vendor commands.
- The colour K-3 III `0189` registration was removed from the legacy USB-SCSI
  camlib so the same physical device is not claimed by two drivers.
- The ability name now says MTP mode.
- Passive VID/PID, interface class, exact DeviceInfo model, firmware, and
  VendorExtensionID discovery precedes every model-specific hardware gate.
- Conflicting identifiers found anywhere in the repository must be resolved
  against passive hardware evidence before implementation proceeds.
- Tests must distinguish repository consistency from real-hardware validation.

## Reproduction and evidence locations

The constrained preview harness is `examples/pentax-safe-preview.c`. It accepts
only the exact K-3 III MTP model, an explicit USB port, and 1 through 500 frames.
It has no configuration setter, still-capture, delete, reset, firmware, or image
retention path.

Detailed records are retained in:

- `docs/pentax/evidence/2026-08-21/H1.1/README.md` — passive discovery;
- `docs/pentax/evidence/2026-08-21/H1.2/README.md` — vendor lifecycle and descriptors;
- `docs/pentax/evidence/2026-08-21/H1.3/README.md` — live-view probes and soak.
- `docs/pentax/evidence/2026-08-21/H1.5/README.md` — bounded focus-drive probe.
- `docs/pentax/evidence/2026-08-21/H1.6/README.md` — Bulb-mode read-only baseline.
- `docs/pentax/evidence/2026-08-21/H1.7/README.md` — K-1 II initial read-only gate.
- `docs/pentax/evidence/2026-08-21/H1.8/README.md` — K-1 II B-mode read-only baseline.

Implementation commits arising directly from these tests are `b72d9cbdc` and
`abf55fbcb`.
