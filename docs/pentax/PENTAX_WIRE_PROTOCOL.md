# Pentax tethering wire protocol

Revision: 2026-08-21

This document distinguishes facts visible in the decompiled IMAGE Transmitter 2
client from facts that still require a USB trace. It is not a claim that the
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
| `25fb:0183` | `PENTAX K-1 Mark II` | 78400 | 1 | Observed-client |

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

## Still capture and transfer

Ordinary still capture uses `InitiateCapture` (`0x9011`) with five parameters:
release mode 0, focus mode, manual-white-balance mode 0, sync mode 0, and
aperture-reset 0. The client uses focus mode 3 with autofocus and 2 without it.

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
