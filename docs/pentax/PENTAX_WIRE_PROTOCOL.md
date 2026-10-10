# Pentax tethering wire protocol

Revision: 2026-08-21

This document distinguishes facts visible in the IMAGE Transmitter 2 client's
published behaviour (observed from its public interfaces) from facts that still
require a USB trace. It is not a claim that the
protocol has passed hardware validation.

## Evidence labels

- **Observed-client**: directly present in `IMAGETransmitter2/MtpDevice.cs`.
- **Inferred-client**: implied by that client's executor or control flow.
- **Unknown-hardware**: must be checked against a USB capture and camera.

A K-3 Mark III running firmware 2.20 was attached on 2026-08-21. Passive USB
enumeration and a forced-generic, read-only DeviceInfo session are recorded in
`evidence/2026-08-21/H1.1/`; later gates remain Unknown-hardware until their own
records pass.

## Supported identity table

| USB VID:PID | Exact DeviceInfo model | Model number | Extension version | Evidence |
|---|---|---:|---:|---|
| `25fb:0189` | `PENTAX K-3 Mark III` | 78420 | 1 | Hardware USB/DeviceInfo + Observed-client |
| `25fb:0183` | `PENTAX K-1 Mark II` | 78400 | 1 | Hardware-confirmed PC-P identity; model number/version remain Observed-client |

Both USB identity and the exact DeviceInfo string must match before vendor
commands are sent. Other Pentax devices retain generic PTP behavior.

On firmware 2.20 the K-3 III must be configured for MTP and reports Microsoft
MTP VendorExtensionID `0x6`; `25fb:018a` is CD-ROM installer mode. Therefore
the guarded model state, not VendorExtensionID `0x0d`, selects Pentax behavior.
Its pre-vendor DeviceInfo omits every Pentax property. After successful enable,
the driver atomically refreshes DeviceInfo; a refresh/fixup failure immediately
rolls vendor mode back and retains safe generic behavior.

## Session lifecycle

`SetVendorMode` (`0x9001`) is a no-data operation with five command parameters:

1. vendor identifier `13`;
2. model number;
3. `1` to enable or `0` to disable;
4. vendor extension version;
5. zero.

The client reads array index 1 from the returned response-parameter array, so
libgphoto2 reads response `Param2` as the function flags and rejects a response
with fewer than two parameters. This is Inferred-client and must be verified by
a handshake trace. Disable is sent during clean exit even if another cleanup
step fails.

### Power-on readiness and wake-up audit

The observed IMAGE Transmitter connection path contains no explicit wake-up
operation. `Connect()` calls the Windows portable-device `Open`, immediately
sets vendor mode with `0x9001`, then reads DeviceInfo, storage IDs, and
`GetAllConditions`. It has no connection delay, retry loop, standard PTP
`PowerDown`/`ResetDevice` call, Pentax shutdown call, or other command before
`0x9001`. Windows WPD may still perform implicit session traffic, so only a USB
trace can establish the exact on-wire sequence.

After connection, IMAGE Transmitter polls `GetAllConditions` every 100 ms. This
can keep an already running session active, but it is not evidence of a command
that can wake a powered-off or non-enumerated camera. Pentax opcode `0x9002` is
named and used as camera shutdown; it is called only from the application's
explicit camera-shutdown UI path and must not be used for recovery.

Current evidence supports a host readiness policy rather than a wake command:
wait for USB enumeration, exclusive ownership, and a settled camera shooting
screen before one vendor-enable attempt. A powered-off camera has no USB/PTP
transport on which a wake command could be delivered. USB remote wakeup, if the
device implements it while suspended, is a USB power-management mechanism and
is not established by this application code.

## Live view

The client writes UINT8 value 1 to device property `0xd035`, then repeatedly
reads opcode `0x9006`. It writes UINT8 value 0 to stop. This sequence and type
are Observed-client. A unit-tested bounds parser requires complete JPEG SOI
(`ff d8`) and EOI (`ff d9`) markers and rejects missing or truncated markers.
Bytes outside those markers are not returned. Whether every supported firmware
returns a bare JPEG or a wrapper is Unknown-hardware.

The libgphoto2 implementation reads and retains the session's original UINT8
value before changing it. It restores that exact value after a frame transport
error, an invalid JPEG frame, or session exit. A failed restoration remains
pending so exit can retry it before vendor mode is disabled.

### AF position (`0xd036`)

The AF-position getter accepts only the exact eight-byte coordinate form
(issue #26). Byte 0 must carry the encoder tag `2` (the same tag the setter
emits); bytes 1–3 are an opaque header and are not validated, because no
hardware observation has fixed their meaning. The little-endian UINT16 x
coordinate sits at byte 4 and y at byte 6. Any other size — including 12- and
16-byte responses — is rejected outright: the trailing bytes' meaning is
Unknown-hardware, so accepting longer frames would let arbitrary payloads pass
as coordinates. A four-byte response is likewise rejected: the earlier
"geometry centre" interpretation had no byte-level evidence, and payloads such
as `ff ff ff ff` must not be accepted as a centre point. Coordinates are
bounds-checked against the parsed live-view geometry
(`x < area_width`, `y < area_height`). All six logged K-3 III observations used
eight-byte responses with `af-response-bytes=8`; no four-byte response has ever
been observed on hardware. The paired setter emits the IT2 form
`{2,0,0,0,Xlo,Xhi,Ylo,Yhi}`.

## Focus drive

IMAGE Transmitter 2 explicitly selects its new focus-fine-control path for the
K-3 Mark III. It reads little-endian `openAvNum` from byte offset 328 of a
`GetAllConditions` (`0x900f`) response and calculates the first displacement as
`(int)(-(openAvNum * 2.5 / 3.0) * requestedDirection)`. The Far UI button uses
requested direction `+1`, producing a negative displacement; Near uses `-1`,
producing a positive displacement. Opcode `0x9017` then carries that signed
displacement in one UINT32 command parameter. The older two-parameter `0x9016`
path is not the K-3 III path.

The official client retries response `0xa00c` up to ten times and multiplies
the displacement by `sqrt(2)` after each failure. Libgphoto2 deliberately does
not reproduce that escalation: each public action makes a fresh status read,
requires at least 332 bytes and a non-zero bounded displacement, sends exactly
one `0x9017`, and returns the first response. On the K-3 Mark III with a
24–70 mm AF lens, offset 328 was 28: `+23` returned `0x2001` and moved focus
nearer; a fresh `-23` returned `0x2001` and moved it farther by approximately
the same small amount. This validates direction and bounded transport, not
calibrated distance or larger displacements.

## GetAllConditions status subset

The candidate parses only source-traced fields and preserves uncorrelated modes
as raw numbers. The mandatory subset ends after offset 504 and therefore
requires at least 508 bytes. Astro limit time at offset 528 is optional and is
read only from responses of at least 532 bytes.

| Offset | Width | Meaning used by the candidate |
|---:|---:|---|
| 24 | 4 | low byte operation state; 49 Astro pre-exposure, 50 Astro main exposure |
| 40 | 4 | raw user mode |
| 104 | 4 | shooting/processing/movie/mirror/interval/multi/self-timer activity bits |
| 168 | 4 | exposure step |
| 184 | 4 | raw exposure mode |
| 272, 276 | 4 each | Bulb timer value numerator/denominator |
| 312 | 4 | ISO |
| 320 | 4 | Astro shift/aperture flags and movement-failed/time-too-long bits |
| 328 | 4 | `openAvNum`, used for minimum focus displacement |
| 492 | 4 | raw drive mode |
| 504 | 4 | Av/Tv/Sv/Xv changeability, task-changing, Bulb timer, GPS, AstroTracer3 bits |
| 528 | 4 | optional Astro limit seconds |

The public `status/pentaxconditions` text widget issues one `0x900f`, has no
setter, and exposes raw IDs where hardware correlation is incomplete. It does
not infer that Astro is ready and does not initiate an exposure.

## Still capture and transfer

This section describes research code, not an ordinary advertised capability.
Default builds return `GP_ERROR_NOT_SUPPORTED` from Pentax preview/capture
dispatch before a vendor capture operation. The code can be reached only in a
separate build compiled with
`LIBGPHOTO2_ENABLE_PENTAX_RESEARCH_CAPTURE=1`; even that build does not advertise
capture, preview, or trigger-capture abilities.

Ordinary still capture uses `InitiateCapture` (`0x9011`) with five parameters:
release mode 0, focus mode, manual-white-balance mode 0, sync mode 0, and
aperture-reset 0. The client uses focus mode 3 with autofocus and 2 without it.

Release mode semantics are only partially correlated. Release mode 0 is the
verified still-capture path above. The Windows Wi-Fi app's bulb flow suggests
release mode 2 opens the shutter for a held (bulb) exposure, paired with
`TerminateCapture` (`0x9012`, same release mode) to close it; this has been
observed in K-3 III traces but is not yet verified on hardware (K-1 II probe
pending). Treat release modes other than 0 as Unknown-hardware until probed.

The host polls `GetAllConditions` (`0x900f`). The client treats little-endian
UINT32 byte offset 32 equal to 1 as a transfer candidate and reads its handle at
byte offset 36. These fields are Observed-client; their model/firmware stability
is Unknown-hardware.

The transfer sequence is:

1. `GetTransferCandidateFileInfo` (`0x900b`) with image type 0.
2. Parse format, quality, image-size code, an 8-bit UTF-16 character count, and
   the UTF-16LE filename beginning at byte 4.
3. Read `GetCamFileOperationCommand` (`0x900c`). Each record contains an 8-bit
   operation and signed little-endian 32-bit operation information.
4. Require operation 1 (begin). Operation 2 finishes; operation 3 requests
   data; operations 4, 5, and 6 seek from begin, current position, and end.
5. For operation 3, request no more than the reported remaining count and an
   8 MiB safety block. `GetTransferFileDataBlock` (`0x900d`) returns its valid
   byte count in response `Param1`. Continue reading blocks for the same file
   command until its reported count is consumed or the camera returns a short
   block; only then request the next file command.
6. Only after a non-empty local file is cached, send
   `DeleteTransferCandidate` (`0x900e`, GETDATA with no parameters) and discard
   its returned data.

The new candidate-transfer path uses `0x900e`; `ReceivedCreatedObject` (`0x9003`)
with an object handle belongs to the client's older standard-GetObject path.
The command grammar and finalization ordering are Observed-client. Timeouts,
strict length checks, a 2 GiB file cap, a command-count cap, cancellation, seek
bounds, and filename path rejection (separators plus reserved `.`/`..`
components) are defensive host policy.

### Host state and error transitions

```text
IDLE -> TRIGGERED -> WAITING -> CANDIDATE -> TRANSFERRING
     -> CACHING -> FINALIZING -> COMPLETE -> IDLE
```

The current state and candidate handle are session-local, never global. Every
error returns to host state IDLE and releases host buffers/files. An initiation
error returns directly from IDLE. Timeout, cancellation, malformed/short status,
or a status transport error can exit WAITING. Candidate metadata errors exit
CANDIDATE. Invalid command ordering, unsupported commands, invalid seeks,
zero/oversized blocks, allocation limits, block/command transport failures,
timeout, and cancellation can exit TRANSFERRING. Local file/cache failures exit
CACHING. A `0x900e` failure exits FINALIZING after the host file has already been
cached.

No unobserved cleanup command is sent on those error paths. Consequently a
camera-side candidate may remain after failures from CANDIDATE onward; this is a
documented hardware question, not a claimed clean cancellation. Successful
finalization alone proves the candidate was released.

On pre-candidate failure the host sends `TerminateCapture` (`0x9012`) with
release mode 0 — the same release mode as the initiating still capture — and
then re-reads `GetAllConditions` to check that activity flags (byte offset 104,
bit 0) have cleared. The abort opcode choice follows the documented
release/close pairing; `InterruptFunction` (`0x9013`, "Green button") is not
used for cancellation because its semantics are Unknown-hardware. Post-abort
quiescence is diagnostic only: a non-clear flag is logged, never retried.

## Open hardware questions

- Determine why a cold camera restart caused `SetVendorMode` to return General
  Error `0x2002` in two fresh sessions after it had succeeded before the restart.
  Establish whether the prerequisite is camera UI state, connection ordering,
  or client-created state. Do not add an automatic retry loop or alternate
  handshake without evidence.
- Confirm response parameters and response codes for vendor-mode enable/disable.
- Record actual DeviceInfo strings for both target firmware families.
- Confirm `0xd035` descriptor datatype and mode restoration.
- Validate JPEG boundaries for 500 preview frames.
- Verify status offsets and candidate-handle value 1 behavior.
- Exercise JPEG and RAW transfers, all emitted file operations, cancellation,
  short final blocks, disconnect, reconnect, and acknowledgement failures.
- Capture descriptors before implementing model-specific configuration values.
