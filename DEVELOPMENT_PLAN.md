# Pentax support: executable development plan

Status: canonical plan, revision 2026-08-21

## Progress ledger

| Date | Work | Result | Evidence / next gate |
|---|---|---|---|
| 2026-08-21 | Canonical plan review and cloud publication | PASS | Commit `9ca537595`; obsolete workspace plans moved to `archive/` |
| 2026-08-21 | M0 repository, tool, and camera inventory | PARTIAL | Repository/tool inventory PASS; target hardware BLOCKED; see `docs/pentax/evidence/2026-08-21/M0.1/` |
| 2026-08-21 | P1 client-side protocol extraction | PARTIAL | Wire contract documented; USB observations remain BLOCKED |
| 2026-08-21 | P2 wrappers and guarded session state | COMPILE PASS | Hardware transaction gate BLOCKED |
| 2026-08-21 | P3 vendor lifecycle | COMPILE PASS | Enable/disable/reconnect gate BLOCKED |
| 2026-08-21 | P4 preview path | COMPILE PASS | 500-frame camera gate BLOCKED |
| 2026-08-21 | P5 capture and transfer state machine | COMPILE PASS | JPEG/RAW/cancel camera gate BLOCKED |
| 2026-08-21 | Obsolete `pentaxmodern` prototype | ARCHIVED | Removed stale build registrations; source retained under workspace `archive/obsolete-source/` and in Git history |
| 2026-08-21 | Pentax parser/transfer-buffer unit tests | PASS | Fresh container compile plus `test-pentax-utils` 1/1 PASS |
| 2026-08-21 | Autotools/source-distribution path | PASS | Removed stale invalid Makefile text; `autoreconf`, configure, and `make dist-xz` PASS; archive contains Pentax utility source |
| 2026-08-21 | Candidate-transfer finalization audit | CORRECTED | New 0x900B–0x900D path now finalizes with GETDATA 0x900E; 0x9003 retained only for legacy object path |
| 2026-08-21 | Multi-block transfer audit | CORRECTED | Operation 3 now consumes repeated bounded 0x900D blocks before requesting the next file command |

Current implementation work does not satisfy the definition of done until the
hardware gates and full-build tests pass. Configuration values are deliberately
not guessed without real `GetDevicePropDesc` evidence.

This is the single source of truth for adding Pentax tethering support to the
libgphoto2 fork and delivering that build through BenroPolarisPatcher. Copies of
this plan elsewhere are informational only.

The primary target is the PENTAX K-3 Mark III colour body (`25fb:018c`). The
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
- Extracted notes: `CameraCapabilities.md`

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
   response handling, cancellation, and 0x9003 acknowledgement.
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

Deliverable: `libgphoto2/docs/pentax/PENTAX_WIRE_PROTOCOL.md`, labeling every
field Observed, Inferred, or Unknown and citing its trace/work-record ID.

Gate P1: no Unknown may affect handshake, preview, or capture.

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
6. acknowledge object (0x9003);
7. interrupt (0x9013);
8. focus only if observed and in scope.

Use the observed data phase, named constants, host-order parameters, existing
buffer ownership, and response-parameter count checks. Propagate non-OK PTP
responses. A local “enabled” flag never substitutes for real lifecycle handling.

Gate P2: each wrapper has a successful hardware record or is removed. Stubs and
uncalled wrappers do not pass.

### P3 — Integrate the camera lifecycle

Primary file: `camlibs/ptp2/library.c`.

1. After session open and DeviceInfo retrieval, match an exact supported model.
2. Enable vendor mode once, store returned flags, then refresh descriptors if P1
   proves that necessary.
3. On failure, retain generic PTP and suppress Pentax-only features.
4. On exit, stop live view, cancel active transfer if required, disable vendor
   mode, and continue cleanup even when one cleanup command fails.
5. Verify reconnect and two simultaneous camera instances.

Gate P3: 50 connect/disconnect cycles, including unplug during initialization,
without crash, stale state, or failure of the next generic connection.

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

### P5 — Implement capture and transfer as one state machine

Document these states and all error transitions in
`libgphoto2/docs/pentax/PENTAX_WIRE_PROTOCOL.md`:

```text
IDLE -> TRIGGERED -> WAITING -> CANDIDATE -> TRANSFERRING
     -> ACKNOWLEDGING -> COMPLETE -> IDLE
```

1. Wire trigger into the correct `camera_trigger_capture()` and/or
   `camera_capture()` semantics.
2. Use reliable observed events; otherwise bounded status polling with a
   deadline and `GPContext` cancellation.
3. Validate metadata lengths and UTF-16 conversion.
4. Stream blocks via `PTPDataHandler` or bounded buffers. Requested length is
   not assumed to equal returned response length.
5. Verify total size and send 0x9003 exactly when observed.
6. Surface the correct file path/event and invalidate relevant caches.

Tests: JPEG, RAW, RAW+JPEG if supported, five sequential captures, full card,
no card, two slots, timeout, cancel, unplug, and hash comparison.

Gate P5: no successful or cancelled transfer leaves a stuck candidate unless a
documented protocol limitation requires reconnect.

### P6 — Add configuration incrementally

One property per commit: ISO, aperture, shutter, white balance, exposure
compensation, focus mode, drive mode, then file format.

For each property:

1. Use a real `submenu` entry and typed get/put pattern from `config.c`.
2. Use the device descriptor. Hard-code only when the descriptor is absent and
   P1 observed every supported value on each claimed model.
3. Respect GetSet and camera mode; omit unsupported widgets.
4. After set, invalidate/refetch and verify round-trip, then restore the original.

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
