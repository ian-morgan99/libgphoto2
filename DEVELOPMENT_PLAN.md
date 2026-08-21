# Pentax support: executable development plan

Status: canonical plan, revision 2026-08-21

All physical-camera activity is also recorded chronologically in
`docs/pentax/REAL_HARDWARE_TEST_LOG.md`. Updating that log is part of completing
any real-hardware test; the plan status table is not a substitute for it.

## Progress ledger

| Date | Work | Result | Evidence / next gate |
|---|---|---|---|
| 2026-08-21 | Canonical plan review and cloud publication | PASS | Commit `9ca537595`; obsolete workspace plans moved to `archive/` |
| 2026-08-21 | M0 repository, tool, and camera inventory | PARTIAL | Repository/tool inventory PASS; target hardware BLOCKED; see `docs/pentax/evidence/2026-08-21/M0.1/` |
| 2026-08-21 | P1 client-side protocol extraction | PARTIAL | Passive USB and limited warm-session observations exist; official-client handshake trace and capture/transfer traces remain BLOCKED |
| 2026-08-21 | P2 wrappers and guarded session state | PARTIAL HARDWARE PASS | `0x9001` and `0x9006` have bounded warm-session evidence; all capture/transfer and dormant wrappers remain hardware-blocked |
| 2026-08-21 | P3 vendor lifecycle | CONTROLLED CHECKPOINT PASS / BLOCKED | Warm-state and 3/3 explicitly ready cold starts passed; two earlier less-controlled restarts returned `0x2002`; exact readiness prerequisite and the 10+10/50-cycle gates remain open |
| 2026-08-21 | P4 preview path | PARTIAL HARDWARE PASS | A 50-frame paced warm-session soak passed with cleanup; cold-start lifecycle and 500-frame gates remain |
| 2026-08-21 | P5 capture and transfer state machine | COMPILE PASS | JPEG/RAW/cancel camera gate BLOCKED |
| 2026-08-21 | Obsolete `pentaxmodern` prototype | ARCHIVED | Removed stale build registrations; source retained under workspace `archive/obsolete-source/` and in Git history |
| 2026-08-21 | Pentax parser/transfer-buffer unit tests | PASS | Fresh container compile plus `test-pentax-utils` 1/1 PASS |
| 2026-08-21 | Autotools/source-distribution path | PASS | Removed stale invalid Makefile text; `autoreconf`, configure, and `make dist-xz` PASS; archive contains Pentax utility source |
| 2026-08-21 | Candidate-transfer finalization audit | CORRECTED | New 0x900B–0x900D path now finalizes with GETDATA 0x900E; 0x9003 retained only for legacy object path |
| 2026-08-21 | Multi-block transfer audit | CORRECTED | Operation 3 now consumes repeated bounded 0x900D blocks before requesting the next file command |
| 2026-08-21 | Live-view ownership audit | CORRECTED | Read and retain original 0xD035 value; restore on frame error, malformed frame, and exit; retry restoration during exit after a transient failure |
| 2026-08-21 | B1 candidate-source provenance and safety | PARTIAL PASS | Polaris `6210741`; clean/dirty/archive/version/security preflights PASS; ARM/QEMU/repack BLOCKED without FwPkt |
| 2026-08-21 | Transfer state machine fault injection | PASS | Callback-driven command/block machine; sequencing, seeks, multi-block, malformed, transport, cancel, timeout, and ownership paths pass ASan/UBSan |
| 2026-08-21 | P6 configuration-path audit | DOCUMENTED | Six required settings already have descriptor-gated ptp2 handlers; vendor structures withheld pending real descriptors; see `docs/pentax/PENTAX_CONFIGURATION.md` |
| 2026-08-21 | B2 runtime-shim policy audit | PARTIAL PASS | Polaris R5-II storage/capturetarget shims now require an exact R5 II abilities model and fail closed for Pentax/unknown cameras; native and ARM policy tests PASS; full loader/QEMU/repack remain BLOCKED without FwPkt |
| 2026-08-21 | Candidate filename security audit | PASS | UTF-16 parser rejects separators, non-ASCII/control input, empty names, overflow, and reserved `.`/`..` path components; regression fixtures pass sanitizers |
| 2026-08-21 | B2 complete-loader cross-build | PASS | Polaris `2b505c7`; full Stage-2 loader plus exact-model policy compiles as ARM EABI5 shared object with `-Werror`; execution/package gates remain BLOCKED without FwPkt |
| 2026-08-21 | Legacy capability-claim audit | CORRECTED | Replaced speculative checkmark table and K-3 III/Monochrome PID conflation with implementation-vs-hardware evidence matrix; original moved to workspace archive |
| 2026-08-21 | Pentax live-view framing audit | CORRECTED | Dedicated bounded JPEG parser now requires complete SOI/EOI and rejects missing/trailing/truncated markers; parser fixtures pass sanitizers |
| 2026-08-21 | H1.1 K-3 III passive/read-only discovery | PASS | Real firmware 2.20: MTP `25fb:0189`, CD-ROM `25fb:018a`, exact DeviceInfo model, Microsoft extension `0x6`, SD1/SD2; no mutation commands; serial redacted |
| 2026-08-21 | K-3 III routing correction | HARDWARE PASS | Moved hardware-proven `0189` from legacy USB-SCSI camlib to ptp2; exact model state gates lifecycle/config routing despite Microsoft MTP extension ID |
| 2026-08-21 | H1.2 vendor lifecycle and descriptors | CONDITIONAL PASS | One warm-state lifecycle exposed and then hid Pentax properties, but cold-start repetition failed with `0x2002`; the result proves one session, not a stable handshake |
| 2026-08-21 | Post-enable DeviceInfo cache audit | CORRECTED | Normal config listing initially saw only pre-vendor properties; candidate now atomically refreshes after enable and rolls vendor mode back on refresh/fixup failure |
| 2026-08-21 | Read-only configuration widgets | PARTIAL PASS | Aperture, exposure compensation, and shutter decode correctly; Pentax white-balance vendor labels required effective-vendor routing correction; ISO/focus remain withheld |
| 2026-08-21 | H1.3 single live-view frame | PASS | Valid 1080x720 JPEG returned; disposable host file only; `0xd035` independently verified restored to 0; see evidence record |
| 2026-08-21 | Safe multi-preview harness | IMPLEMENTED | Fixed-model, explicit-port, 1..500-frame tool retains no images, validates complete JPEGs in memory, exposes no setters/capture/delete path, and always runs camera exit cleanup |
| 2026-08-21 | Initial multi-preview timing probe | PARTIAL | Two valid frames followed by frame-3 preview error under an unpaced loop; cleanup ran; harness now uses conservative 250 ms pacing and reports exact failure stage |
| 2026-08-21 | Paced multi-preview probe | PASS | 10/10 complete in-memory JPEGs with 250 ms pacing; no retained images; explicit camera exit succeeded |
| 2026-08-21 | Paced preview soak | PASS | 50/50 complete in-memory JPEGs with 250 ms pacing; no retained images or shutter/card operations; strengthened camera exit reported cleanup success |
| 2026-08-21 | Exit error propagation audit | CORRECTED | Pentax exit continues vendor/session cleanup but now preserves a live-view restoration failure instead of allowing successful vendor disable to mask it |
| 2026-08-21 | First cold-restart config read | FAIL-CLOSED | Camera returned `0x2002` to vendor enable after power cycle; driver sent no later Pentax opcode and exposed generic MTP only; restart-state requirement remains unknown |
| 2026-08-21 | Controlled clean-start shutter read | PASS (3/3) | All first enables succeeded with flags 0; read-only `0x1014` descriptor for `0xd00f` reported current 1 second and 55 choices; all disable/exits succeeded; exact readiness prerequisite and larger lifecycle gates remain open |
| 2026-08-21 | Controlled warm reconnect shutter read | PASS (3/3) | Three independent powered-on sessions each performed one enable, one `0xd00f` descriptor read, one disable, and clean exit; current value remained 1 second; larger lifecycle gates remain open |
| 2026-08-21 | IMAGE Transmitter wake-up audit | NO WAKE COMMAND FOUND | Client opens WPD and immediately sends `0x9001`; post-connect `0x900f` polling maintains a live session; `0x9002` is explicit shutdown only; USB trace still required to reveal WPD-managed session traffic |
| 2026-08-21 | Manual-mode full config snapshots | READ-ONLY PASS | Three 777-line trees were byte-identical after serial/clock/battery normalization; zero setter opcodes; stable decoded and raw vendor-property baseline recorded in H1.4 |
| 2026-08-21 | First bounded K-3 III focus drive | INCONCLUSIVE / STOPPED | Source-derived minimum Near action made no retries but returned `GP_ERROR_IO`; Far was correctly withheld; add stage-specific raw response logging and record AF/MF plus observed movement before another probe |
| 2026-08-21 | Second bounded Near diagnostic | INCONCLUSIVE / STOPPED | Repeated `GP_ERROR_IO` before the instrumented `0x900f`/`0x9017` calls; Far withheld; harness now labels init, action retrieval, and setter stages before any third attempt |
| 2026-08-21 | Final stage-labelled focus diagnostic | BLOCKED AT INIT | Failed at `camera-init` before action retrieval or the focus setter; no `0x900f`/`0x9017` focus-path evidence and no Far command; resume lifecycle/recovery diagnosis before focus work |
| 2026-08-21 | Init-only check after clean power cycle | FAIL / STOPPED | Camera fully disappeared and re-enumerated at a new address after normal-screen readiness, but exact-model init-only harness still returned `GP_ERROR_IO`; instrument initialization substages/raw responses before more hardware commands |
| 2026-08-21 | Container isolation correction | PASS | Combining bus enumeration visibility with an explicit grant for the single USB node produced clean init, vendor flags 0, cleanup, and zero retries; prior init failures were host device-cgroup artifacts |
| 2026-08-21 | Correctly isolated minimum Near | HOST CRASH AFTER TRANSPORT | No transport error appeared before a null `alreadyset` dereference after the focus call; fix compiles and passes focused plus ASan/UBSan tests; record physical observation before Far or repeat |
| 2026-08-21 | Correctly isolated Near observation | NO PHYSICAL MOVEMENT | Operator saw and heard no lens movement; instrument actual `openAvNum`, computed displacement, and raw response before direction reversal or any increase |
| 2026-08-21 | Instrumented minimum Near | TRANSPORT PASS / PHYSICAL PENDING | `openAvNum=28` produced source-derived displacement `+23`; K-3 III returned PTP OK `0x2001`, zero retries, clean exit; record movement observation before Far or increase |
| 2026-08-21 | Minimum Near physical direction | PASS | Operator confirmed `0x9017(+23)` moved the 24–70 focus nearer; permits one equal source-derived Far reversal, still with no retry/escalation |
| 2026-08-21 | Minimum Far reversal | TRANSPORT PASS / PHYSICAL PENDING | Fresh `openAvNum=28` produced `-23`; camera returned PTP OK `0x2001`, zero retries, clean exit; record visible Far movement before closing bidirectional gate |
| 2026-08-21 | Bounded bidirectional focus | PASS (QUALITATIVE) | `+23` moved nearer and `-23` moved farther by approximately the same small amount; both returned `0x2001`, zero retries, clean exits; no calibrated-distance or larger-step claim |
| 2026-08-21 | Bulb-mode full config snapshots | READ-ONLY PASS (2/2) | Both returned 700 lines; exact audit sent zero setters/focus/capture; `0xd00f` becomes a 1–600 s timer domain (current 300), while `0xd013` narrows to 3 choices/current 0; see H1.6 |
| 2026-08-21 | B-mode ISO 3200→1600→restore | INTERRUPTED BEFORE RESTORE | PTP read-back and camera display confirmed `0xd01e` is ISO and changed to 1600; camera then lost USB/power with a flat battery before restore container launch, so no 3200 restore was sent; restore is the next mandatory action after charging |
| 2026-08-21 | H0 interrupted ISO restoration | PASS | After charging, display and a guarded PTP read confirmed retained 1600; exactly one write restored advertised value 3200; a fresh PTP session and operator display confirmation both reported 3200; USB ownership was released cleanly |
| 2026-08-21 | B/Astro condition parser and read-only status | OFFLINE PASS / HARDWARE PENDING | Bounded `0x900f` parser covers activity, raw modes, ISO, Bulb timer, Astro phase/errors/limit, and changeability flags; 508/532-byte boundary fixtures and ptp2 build pass; one status widget performs one read and no write |

Current implementation work does not satisfy the definition of done until the
hardware gates and full-build tests pass. Configuration values are deliberately
not guessed without real `GetDevicePropDesc` evidence.

## Current hard stop

The interrupted ISO restoration is closed: the original 3200 value was restored
and confirmed through both a fresh PTP read and the camera display. The next
hardware task is H0.1's read-only condition snapshot. Until that closes:

- do not write another setting, drive focus, start preview, release the shutter,
  capture, transfer, delete, reset USB, or run a Polaris camera test;
- a fresh failed `0x9001` session remains fail-closed: send no later Pentax
  opcode and do not retry in place;
- do not claim reconnect stability, capture, transfer, or Astro exposure support.

The earlier `0x2002` diagnosis was partly confounded by incomplete container
isolation. Controlled clean starts subsequently passed 3/3, but the plan's
10+10 and 50-cycle lifecycle gates remain open. Both facts must remain visible;
neither turns the other into evidence that never happened.

### Immediate execution queue

Complete these cards in order. A junior agent must stop on the first failed or
missing exit criterion and must not begin a later card.

#### H0 — Close the interrupted ISO restore (first powered-camera action)

1. Confirm the camera display still shows ISO 1600 and record discrepancies.
2. Establish exclusive USB ownership using both the bus mount and the explicit
   single-device grant; never grant the whole USB bus through the device cgroup.
3. Open one exact-model session, read `0xd01e`, and require current value 1600.
4. Set only `0xd01e` to the recorded original 3200; read it back once.
5. Ask the operator to confirm ISO 3200 on the camera, then exit and verify
   vendor disable/session cleanup. On any mismatch or non-OK response, stop.
6. Update H1.6, the real-hardware log, and this ledger in the same commit.

Exit: **PASS 2026-08-21.** PTP and display both showed 3200 and cleanup
succeeded. See H1.6 and the real-hardware log.

#### H0.1 — Validate the condition snapshot without exposure

1. Keep the dial at B and make one named `status/pentaxconditions` read.
2. Audit the transport log: exactly one `0x900f` for this widget and no setter,
   focus, preview, capture, transfer, delete, or reset opcode.
3. Correlate only visible/static facts: ISO, Bulb timer enabled/value, and raw
   exposure/drive IDs. Do not attempt to enter Astro exposure phases.
4. Repeat once after a clean session. Store sanitized output and exact response
   length; update the protocol and hardware records.

Exit: 2/2 reads parse successfully, observable values agree, cleanup succeeds,
and unknown mode IDs remain numeric.

#### R0 — Contain unverified public capabilities (offline only)

1. Remove `PTP_CAP` and `PTP_CAP_PREVIEW` from both new Pentax ability rows.
2. Compile-gate the Pentax capture/transfer dispatch off by default while
   retaining parser/state fixtures as offline research code.
3. Confirm direct calls fail with `GP_ERROR_NOT_SUPPORTED` unless the research
   build flag is explicitly enabled; no runtime preference may silently enable
   it.
4. Build both `ptp2` and legacy `pentax` camlibs; run the full available test
   suite and focused sanitizer fixtures.

Exit: ordinary discovery advertises neither preview nor capture for these rows,
generic file access remains available, and no camera is required.

#### R1 — Establish the official cold-start handshake (capture only)

1. Record camera firmware, MTP setting, visible screen, mode dial, power-cycle
   state, USB enumeration time, and host ownership.
2. From a cold camera, capture one IMAGE Transmitter connection and clean
   disconnection without changing a camera setting or taking a photograph.
3. Redact serial/string descriptors and store the capture outside Git; commit a
   transaction table and SHA-256 only.
4. Tabulate USB-level `GetDeviceInfo`, `OpenSession`, `0x9001`, response
   parameters, any intervening commands, descriptor refresh, disable, and
   `CloseSession` in exact order.

Stop: if the official client also receives `0x2002`, do not alter the driver;
record the camera/UI state mismatch for user review.

Exit: the first successful official cold-start handshake is reproducible twice
and its wire order is unambiguous.

#### R2 — Reconcile candidate ordering (one variable per probe)

1. Diff candidate and official transaction tables, including whether WPD opened
   a PTP session implicitly.
2. Write a fixture for the expected command order and response-parameter count.
3. Make the smallest ordering/state change supported by the trace.
4. Build and test offline before connecting hardware.
5. Run one candidate cold-start connection. On any non-OK response, stop; do not
   try another ordering in the same session.

Exit: 3/3 cold-start and 3/3 warm candidate lifecycles pass with exact cleanup,
and a forced `0x2002` fixture proves that no subsequent Pentax opcode is sent.

#### R3 — Implement bounded recovery

1. Represent lifecycle as explicit states: generic, enabling, enabled,
   restoring, disabling, and failed. Reset all fields on initialization and
   after exit.
2. On enable failure, preserve generic access and an actionable diagnostic.
3. Permit one retry only after the old session is closed, USB ownership is
   released, a new connection event occurs, and identity is read again.
4. Unit-test enable failure, descriptor-refresh failure, live-view restoration
   failure, disable failure, disconnect at each state, and two camera instances.

Exit: R2's 10+10 lifecycle gate and then P3's 50-cycle gate pass. Logs prove no
in-place retry, USB reset, speculative parameters, stale state, or serial leak.

#### R4 — Resume read-only setting enumeration

1. Use the successful cold-start lifecycle from R3.
2. Capture three raw descriptor snapshots as specified in P6.
3. Produce a table with property code, datatype, GetSet, default, current,
   form, choices/range, visible camera value, and confidence label.
4. Update `PENTAX_CONFIGURATION.md`, the real-hardware log, and the evidence
   matrix in the same commit. Do not add a setter.

Exit: all stable descriptors are documented; dynamic and unexplained values are
isolated; every unknown remains numeric. Setting writes require a separate,
explicitly approved P6 task.

This is the single source of truth for adding Pentax tethering support to the
libgphoto2 fork and delivering that build through BenroPolarisPatcher. Copies of
this plan elsewhere are informational only.

The primary target is the PENTAX K-3 Mark III colour body in its required MTP
mode (`25fb:0189`). The
PENTAX K-1 Mark II (`25fb:0183`) is the second validation body. Adding a USB ID
does not mean that a camera has vendor support. Unknown Pentax bodies must retain
safe generic PTP behaviour.

## 1. Outcome and definition of done

The work is complete only when all applicable rows below have recorded evidence.

| Capability | Native Linux acceptance test | Polaris acceptance test |
|---|---|---|
| Detection | Exact model selected by `gphoto2 --auto-detect` | `pgphoto` selects the new `ptp2` camlib |
| Generic files | One named JPEG and one named RAW download with matching hashes | Same files arrive through the Polaris workflow |
| Configuration | ISO, aperture, shutter, white balance, and focus mode can be read; supported writable values round-trip | Polaris changes every setting it actually exposes |
| Live view | 500 consecutive `--capture-preview` frames, then stop/restart and reconnect | 30 minutes of preview, stop/restart, and cold reconnect |
| Capture | Trigger, completion detection, download, and acknowledgement work for JPEG and RAW | Five consecutive captures, including reconnect and cold boot |
| Status | Validated battery, capture state, transfer candidate, and card/slot values update | Required status reaches the UI or is documented as unused by `pgphoto` |
| Safety | Existing tests pass; a non-Pentax PTP body still initializes | Stock restore image and on-device revert both verified before flashing |

“Works” means a command, result, log, camera/firmware identity, and artifact hash
are recorded under `libgphoto2/docs/pentax/evidence/<date>/`. Compilation alone is never a
feature acceptance test.

## 2. Repository map and authority

- Workspace and extracted reference: `/home/ian/Documents/VSCodeProjects/LibGphoto2`
- libgphoto2 repository: `/home/ian/Documents/VSCodeProjects/LibGphoto2/libgphoto2`
- Polaris repository: `/home/ian/Documents/VSCodeProjects/BenroPolarisPatcher`
- Decompiled reference: `ImageTransmitter2/IMAGETransmitter2/MtpDevice.cs`
- Evidence-status matrix: `CameraCapabilities.md`
- Archived extracted notes: workspace `archive/CameraCapabilities.extracted-obsolete-20260821.md`

The decompiled application is evidence about one host implementation, not a
wire-protocol specification. USB captures and real camera responses outrank it.
Do not copy proprietary source into shipping code. Implement independently from
observed facts and retain provenance in research notes.

Before every task:

1. Run `git status --short` in each Git repository that will be touched.
2. Preserve unrelated changes. `camlibs/ptp2/library.c` may already contain user
   changes.
3. Create one focused commit per completed task card. Do not commit captures,
   firmware, serial numbers, or proprietary binaries.
4. Stop at a hardware gate when its evidence is unavailable. Do not replace an
   observation with guessed constants or a stub.

## 3. Corrections to the obsolete plan

These are non-negotiable design constraints.

1. libgphoto2 initiates PTP. Pentax operations are host-side wrappers that
   construct `PTPContainer` and call `ptp_transaction()`; they are not
   responder-side `case` handlers. Do not invent `ptp_get_data()` or
   `ptp_emit_data()` helpers.
2. `SetPentaxVendorMode` is observed as a no-data command with five parameters:
   `13, model_no, enable, vendor_ext_version, 0`. Its function flag is a response
   parameter, not a data payload. Its exact response index is unverified until
   Task P1 records a trace.
3. Operation and device-property definitions belong in `camlibs/ptp2/ptp.h`,
   beside other vendor constants—not in `mtp.h`.
4. `params->deviceinfo.Model` is the cached model. Model is not fetched through
   a fictional `PTP_DPC_Model` property.
5. Configuration uses the existing `struct menu`, `struct submenu`, typed value
   tables, and getter/setter callbacks in `config.c`. Do not invent
   `GP_CAPTURE_*` constants or edit `gphoto2-port.h` for widget names.
6. Vendor state is per camera/session in `PTPParams`; static global state is
   forbidden.
7. Property datatypes come from captured `GetDevicePropDesc` responses. A C#
   `byte[]` does not establish the PTP datatype.
8. Status offsets are byte offsets. Every read is little-endian and
   bounds-checked. Conflicting documented offsets remain unresolved until traced.
9. Capture is a state machine including completion detection, block-length
   response handling, cancellation, and finalization. New candidate transfer
   uses `0x900e`; `0x9003` is reserved for the separately observed legacy
   GetObject path.
10. Hardware validation occurs after each new transaction, not after all code is
    written.

## 4. Work records required from every task

Create `libgphoto2/docs/pentax/evidence/<YYYY-MM-DD>/<task-id>/README.md`
inside the libgphoto2 Git repository containing:

- source commit and dirty-state summary;
- camera model, firmware, VID:PID, USB mode, host OS, and library version;
- exact commands and relevant environment variables;
- expected and actual results;
- hexadecimal response codes;
- hashes and sizes of generated/downloaded files;
- redacted log/capture filenames;
- PASS, FAIL, or BLOCKED and the reason.

Large captures stay outside Git. Commit only a redacted transaction table and
the capture’s SHA-256.

Every hardware command must also be entered in
`docs/pentax/REAL_HARDWARE_TEST_LOG.md`, including failed and aborted probes.
Record the camera's visible UI state, mode dial, USB setting, whether it was
power-cycled, the time since enumeration, USB owner check, session ordering,
request parameters, response code, and which cleanup operations actually ran.

## 5. Milestones and task cards

Tasks are ordered by dependency. Offline implementation may proceed when a
hardware gate is unavailable, but the affected task remains BLOCKED and no
dependent capability may be called accepted, releasable, or production-ready.
Before release, run the hardware gates in order and return to implementation on
the first failure.

### M0 — Freeze baselines and targets

#### M0.1 Record repository baselines

1. Record `git rev-parse HEAD`, branch, remotes, and `git status --short` for
   both Git repositories.
2. Record compiler, Meson, Autotools, gphoto2 CLI, Docker, and QEMU versions.
3. Inventory available cameras and firmware without assuming both target bodies
   are present.
4. Copy no firmware into the libgphoto2 repository.

Exit: the work record identifies exactly what can be tested. If the K-3 III
colour body is unavailable, mark its hardware tasks BLOCKED; do not silently
substitute the Monochrome model.

#### M0.2 Build the unmodified native baseline

From `libgphoto2` use an out-of-tree build:

```sh
meson setup build-pentax-baseline
meson compile -C build-pentax-baseline
meson test -C build-pentax-baseline --print-errorlogs
```

If Meson dependencies require the documented Autotools route, record why and
use `autoreconf -is`, a separate build directory, `../configure`, `make -j2`,
and `make check`.

Exit: baseline build/test results are recorded before Pentax feature changes.

### P1 — Produce a wire-protocol specification

Protocol extraction is kept separate from feature commits. If hardware is
unavailable, client-derived fields must remain labeled Inferred/Unknown and
must be guarded and validated before use; they cannot satisfy this milestone's
gate.

#### P1.1 Record generic PTP identity and descriptors

For each available target body:

1. Put it in the exact USB mode used for tethering.
2. Record `lsusb -nn`, `gphoto2 --auto-detect`, `--summary`, `--list-config`,
   and `--list-all-config` with debug logging.
3. Record DeviceInfo vendor extension ID/version, exact model string, operations,
   events, and properties.
4. Save every target property descriptor: datatype, GetSet, default/current
   value, and enum/range form.
5. Download one selected JPEG and RAW—not `--get-file all`—and compare SHA-256
   hashes with card-reader copies.

Exit: USB IDs and baseline generic capabilities are established facts.

#### P1.2 Capture Image Transmitter transactions

Capture and tabulate separately:

- connect, session open, DeviceInfo, vendor-mode enable;
- vendor-mode disable and clean disconnect;
- PC live-view start, three frames, stop;
- autofocus and each observed focus-drive operation;
- one JPEG capture/transfer and one RAW capture/transfer;
- cancellation during capture or transfer;
- each configuration property in the first slice;
- GetAllConditions in idle, live-view, capture, transfer, and card-slot states.

Record opcode, data-phase direction, every command parameter, response code,
every response parameter, data size, and ordering. Redact identifiers.

Handshake capture is the first mandatory trace. Determine whether Windows WPD
sends `OpenSession` on the wire before `0x9001`; the decompiled `Connect()` call
sequence (`OpenDevice`, then vendor mode) does not establish USB transaction
ordering because WPD may manage PTP sessions internally. Do not test an
out-of-session vendor command from libgphoto2 until the official-client trace
establishes that ordering.

#### P1.3 Resolve protocol questions

| Question | Required evidence |
|---|---|
| Which response parameter carries `funcAvailFlag`? | Handshake trace plus C# executor indexing |
| When is model number selected? | Exact mapping and handshake trace |
| What enables/stops PC live view? | Full sequence and restored values |
| Is a frame raw JPEG or wrapped? | Hex header, JPEG bounds, decoder result |
| How is transfer completion signalled? | Final data and response lengths, events/status |
| When is 0x9003 sent? | Successful and cancelled transfer traces |
| Which status layout applies per model/version? | Blob lengths and changing byte ranges |
| Which properties are writable in which modes? | Descriptors and set attempts |
| Why does cold-start `0x9001` return `0x2002`? | Repeated official-client and candidate traces with camera UI state and session ordering |

Deliverable: `libgphoto2/docs/pentax/PENTAX_WIRE_PROTOCOL.md`, labeling every
field Observed, Inferred, or Unknown and citing its trace/work-record ID.

Gate P1: no Unknown may affect handshake, preview, or capture. This is a hard
dependency: code may parse captured fixtures offline, but no dependent vendor
transaction may be sent to hardware while the handshake gate is open.

### P2 — Add a minimal protocol layer

#### P2.1 Add constants and per-camera state

Files: `camlibs/ptp2/ptp.h`, plus an optional new `pentax.c` and its build lists.

1. Add constants only for observed operations and properties.
2. Define compact per-camera state in `PTPParams`: matched model, model number,
   extension version, returned flags, vendor/live-view state, and transfer state.
3. Use exact normalized DeviceInfo model plus VID:PID where required. Unknown
   bodies receive no vendor commands.

Tests: warning-clean compile and model-table fixture tests. Confirm two camera
instances cannot share state.

#### P2.2 Implement transaction wrappers

Wrappers contain only packing, `ptp_transaction()`, response extraction, and
bounds checking. Implement and hardware-test one at a time:

1. vendor-mode enable/disable (0x9001);
2. raw GetAllConditions (0x900F);
3. live-view frame (0x9006);
4. initiate/terminate capture (0x9011/0x9012);
5. candidate info, file command, and block transfer (0x900B–0x900D);
6. finalize new candidate transfer (GETDATA 0x900E); retain 0x9003 only for the
   separately observed legacy GetObject path;
7. interrupt (0x9013);
8. focus only if observed and in scope.

Use the observed data phase, named constants, host-order parameters, existing
buffer ownership, and response-parameter count checks. Propagate non-OK PTP
responses. A local “enabled” flag never substitutes for real lifecycle handling.

Gate P2: each wrapper has a successful hardware record or is removed. Stubs and
uncalled wrappers do not pass.

Current corrective task: inventory every `ptp_pentax_*` wrapper. Keep
`SetVendorMode` and live-view-frame wrappers as conditional research code because
they have bounded hardware evidence. Remove or compile-gate the dormant
shutdown, legacy-object, sub-image, main-image, terminate, interrupt, and focus
wrappers until individually observed. Compile-gate the still-capture/transfer
chain so ordinary libgphoto2 callers cannot reach it before P5 passes.

### P3 — Integrate the camera lifecycle

Primary file: `camlibs/ptp2/library.c`.

1. Establish official-client wire ordering from a redacted USB trace.
2. Build a test matrix containing cold power-on, warm reconnect, official-client
   clean disconnect followed by candidate connection, camera shooting/playback/
   menu state, and relevant mode-dial state. Change one factor per test.
3. After the proven session ordering and DeviceInfo retrieval, match an exact
   supported model and send vendor enable once.
4. Store returned flags only after validating response count/code, then refresh
   descriptors if the trace and camera evidence require it.
5. On `0x2002` or any failure, clear Pentax lifecycle/live-view/transfer state,
   retain generic PTP, suppress every Pentax-only feature, and emit an actionable
   diagnostic containing no serial data.
6. Recovery is bounded: release the session and USB ownership; allow at most one
   delayed reconnect after a new connection event and fresh identity read. Never
   loop, automatically USB-reset, power-cycle, or try alternate parameters.
7. On exit, restore live view first; disable vendor mode only if enable succeeded;
   close the PTP session even if cleanup failed, preserving the first meaningful
   cleanup error.
8. Verify reconnect and two simultaneous camera instances.

Gate P3: first pass 10/10 cold-power-on connections and 10/10 warm reconnects;
then 50 connect/disconnect cycles including rejected enable and unplug during
initialization, without crash, hidden retries, stale state, or failure of the
next generic connection. Confirm with logs that no later Pentax opcode follows a
failed enable.

### P4 — Implement live view as the first vertical slice

Integrate with the Pentax branch of `camera_capture_preview()` and only the
existing stream-preview path if required.

1. Save original live-view property values.
2. Apply the observed start sequence with bounded retries.
3. Fetch and validate the observed frame container; never scan unbounded memory
   for JPEG markers.
4. Set the correct MIME type in `CameraFile`.
5. Restore state on completion, cancellation, error, and exit.

Tests: 500 decoded frames; 20 start/stop cycles; cancellation and unplug; still
capture after preview; no Pentax branch for unsupported/non-Pentax devices.

Gate P4: all tests pass on the primary body. Do not infer secondary support.
P4 cannot resume until P3 passes from cold power-on.

### P5 — Implement capture and transfer as one state machine

Document these states and all error transitions in
`libgphoto2/docs/pentax/PENTAX_WIRE_PROTOCOL.md`:

```text
IDLE -> TRIGGERED -> WAITING -> CANDIDATE -> TRANSFERRING
     -> CACHING -> FINALIZING -> COMPLETE -> IDLE
```

1. Wire trigger into the correct `camera_trigger_capture()` and/or
   `camera_capture()` semantics.
2. Use reliable observed events; otherwise bounded status polling with a
   deadline and `GPContext` cancellation.
3. Validate metadata lengths and UTF-16 conversion.
4. Stream blocks via `PTPDataHandler` or bounded buffers. Requested length is
   not assumed to equal returned response length.
5. Verify total size, cache the complete host file, and finalize with `0x900e`
   only after the P1 trace confirms its response/data behavior. Use `0x9003`
   only if separately implementing the observed legacy GetObject path.
6. Surface the correct file path/event and invalidate relevant caches.

Tests: JPEG, RAW, RAW+JPEG if supported, five sequential captures, full card,
no card, two slots, timeout, cancel, unplug, and hash comparison.

Gate P5: no successful or cancelled transfer leaves a stuck candidate unless a
documented protocol limitation requires reconnect.

Before P5 hardware work, remove still-capture capability advertising. Restore
it only in the same commit that records a complete JPEG vertical-slice pass;
RAW and RAW+JPEG remain separately gated. Never use capture itself as a probe to
discover whether the handshake succeeded.

### P6 — Add configuration incrementally

One property per commit: ISO, aperture, shutter, white balance, exposure
compensation, focus mode, drive mode, then file format.

For each property:

1. Require a successful cold-start P3 session. A generic-only configuration
   tree after failed vendor enable is a valid fail-closed result, not evidence
   that vendor properties disappeared from the firmware.
2. Capture two read-only raw descriptor snapshots in one session and a third in
   a fresh session. Normalize only documented dynamic fields such as clock and
   battery before comparing them; explain every other difference.
3. Compare each current value with the camera display without changing it.
4. Use a real `submenu` entry and typed get/put pattern from `config.c`.
5. Use the device descriptor. Hard-code only when the descriptor is absent and
   P1 observed every supported value on each claimed model.
6. Respect GetSet and camera mode; omit unsupported widgets.
7. A write test requires explicit approval and a pre-recorded restoration plan.
   Change one reversible setting by one supported step, refetch it, compare the
   display, restore the exact original value, refetch, and power-cycle to prove
   restoration. Abort further writes on any discrepancy.

Gate P6: the five required settings meet the definition of done. Record genuine
read-only exceptions instead of forcing writes.

### P7 — Parse and expose only validated status

Create bounds-checked little-endian readers. Each field definition states model
layout/version, byte offset, width, signedness, mask/enum, minimum blob length,
and trace evidence.

Initially parse transfer flag/handle, capture state required by P5, confirmed
battery, and confirmed card/activity/current slot. Expose only through existing
summary/config semantics with compatible units. Do not invent vendor property
codes for cached status. Astrotracer and pixel shift are deferred.

Gate P7: fixtures cover short blobs, known layouts, signed values, and state
transitions; hardware values agree with visible camera state.

### Q1 — Native quality and regression gate

```sh
meson compile -C build-pentax
meson test -C build-pentax --print-errorlogs
```

Also run documented style checks, sanitizers on fixtures, and Valgrind on CLI
operations where practical. Test an unknown Pentax in generic mode and one
non-Pentax PTP camera.

Review checklist:

- no globals for session state;
- no unchecked blob/response reads or camera-sized unbounded allocation;
- one owner per returned buffer;
- deadlines and cancellation in every loop;
- no model-prefix matching;
- no logs containing serials or image contents;
- stable public widget names once Polaris depends on them.

Gate Q1: clean build/tests, review checklist, and hardware records accompany the
candidate commit.

### B1 — Make BenroPolarisPatcher consume the candidate source

The patcher currently downloads a released archive. It must test this exact fork.

1. Add `--libgphoto2-source <directory-or-archive>` to shell and PowerShell entry
   points, mutually exclusive with release download.
2. Copy into the Docker context or mount read-only; record Git commit and dirty
   diff hash in output metadata.
3. Update `container/build_ptp2.sh` and `build_fullstack.sh` to use it without
   reapplying release-specific Pentax edits.
4. Preserve and regression-test the release-download path.
5. Fail closed for missing source, dirty source without explicit opt-in, or an
   unexpected build version.
6. Include corresponding LGPL source and diff in the output source offer; never
   include decompiled code.

Gate B1: a dry run proves `ptp2.so` contains a candidate-only marker and metadata
names the exact commit. The ordinary 2.5.34 build still passes its baseline.

### B2 — Polaris compatibility before flashing

Use full-stack mode first because it is hardware-verified. `ptp2-only` is a
separate target, not an assumed fallback.

1. Run existing ELF/ABI, glibc, `DT_NEEDED`, symbol, `_Camera` size, patch-site,
   ownership, hash, and UBIFS `space_fixup` checks.
2. Run QEMU/self-tests and make failures fatal for candidates. QEMU proves
   loading/registration, not USB.
3. Trace `pgphoto` calls, widget names, capture target, paths, and events and
   compare them to P4–P6.
4. Decide from evidence whether Canon-specific shims must be disabled for Pentax;
   default them off for Pentax until tested.
5. Build stage2 and verify install/restore without modifying bootloader, kernel,
   or unrelated appfs files.

Gate B2: package-diff allowlist, QEMU logs, provenance, and verified on-device
revert exist before flashing.

### B3 — On-device and firmware validation

Mandatory order:

1. Keep and hash the stock `FwPkt`.
2. Install reversible stage2 on-device.
3. Test detection, UI settings, preview, capture, download, stop, reconnect, and
   cold boot with logs.
4. Revert stage2 and prove stock operation.
5. Build the flashable image and inspect its file-level diff allowlist.
6. Flash only the documented firmware version with recovery media ready.
7. Repeat acceptance tests, then restore stock once as a recovery rehearsal.

Only real-device results pass B3; cross-build, QEMU, and repack do not.

## 6. Scope control and failure policy

Deferred: unobserved opcodes, “all D0xx” properties, movie transfer,
composition adjustment, dust reduction, Astrotracer, pixel-shift control, and
LS/G900SE support. Each requires a new traced vertical slice.

Do not revive `pentaxmodern` unless evidence shows ptp2 cannot express the
protocol. Do not change public APIs without upstream design review.

- Unsupported feature: omit it and retain generic PTP.
- Vendor enable failure: remain generic and issue no later vendor commands.
- Automatic recovery: clear state and release ownership; retry only once after
  a new connection event, never in-place and never with speculative parameters.
- Malformed response: return an error without partial parsing.
- Timeout/cancel: use only cleanup proven by P1 and report reconnect needs.
- Polaris verification failure: produce no flashable output.
- Missing hardware: mark BLOCKED; simulation never passes a hardware gate.

## 7. Final release evidence

Link the native and Polaris commits, redacted wire specification, build/test
logs, camera matrices with untested cells, stress results, file hashes, patcher
provenance/static report, install/revert/flashed recovery results, limitations,
and an upstreamable commit series.

Only then may documentation say “supported” or “complete.”
