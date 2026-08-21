# Pentax tethering wire protocol

Revision: 2026-08-21

This document distinguishes facts visible in the decompiled IMAGE Transmitter 2
client from facts that still require a USB trace. It is not a claim that the
protocol has passed hardware validation.

## Evidence labels

- **Observed-client**: directly present in `IMAGETransmitter2/MtpDevice.cs`.
- **Inferred-client**: implied by that client's executor or control flow.
- **Unknown-hardware**: must be checked against a USB capture and camera.

No Pentax camera was attached on 2026-08-21, so every on-device result remains
Unknown-hardware.

## Supported identity table

| USB VID:PID | Exact DeviceInfo model | Model number | Extension version | Evidence |
|---|---|---:|---:|---|
| `25fb:018c` | `PENTAX K-3 Mark III` | 78420 | 1 | Observed-client |
| `25fb:0183` | `PENTAX K-1 Mark II` | 78400 | 1 | Observed-client |

Both USB identity and the exact DeviceInfo string must match before vendor
commands are sent. Other Pentax devices retain generic PTP behavior.

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
are Observed-client. The returned data is searched with bounded JPEG framing;
whether every supported firmware returns a bare JPEG or a wrapper is
Unknown-hardware.

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
   byte count in response `Param1`.
6. Only after a non-empty local file is cached, send
   `DeleteTransferCandidate` (`0x900e`, GETDATA with no parameters) and discard
   its returned data.

The new candidate-transfer path uses `0x900e`; `ReceivedCreatedObject` (`0x9003`)
with an object handle belongs to the client's older standard-GetObject path.
The command grammar and finalization ordering are Observed-client. Timeouts,
strict length checks, a 2 GiB file cap, a command-count cap, cancellation, seek
bounds, and filename path rejection are defensive host policy.

## Open hardware questions

- Confirm response parameters and response codes for vendor-mode enable/disable.
- Record actual DeviceInfo strings for both target firmware families.
- Confirm `0xd035` descriptor datatype and mode restoration.
- Validate JPEG boundaries for 500 preview frames.
- Verify status offsets and candidate-handle value 1 behavior.
- Exercise JPEG and RAW transfers, all emitted file operations, cancellation,
  short final blocks, disconnect, reconnect, and acknowledgement failures.
- Capture descriptors before implementing model-specific configuration values.
