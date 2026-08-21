# IMAGE Transmitter 2 setting transaction path

Revision: 2026-08-21

This document is the normative source audit for Pentax setting writes. It traces
the decompiled IMAGE Transmitter 2 (IT2) implementation end to end and separates
facts from remaining hypotheses. Member names below are stable source anchors;
line numbers are deliberately not normative.

## Connection sequence

`MainWindow` assigns the exact model before calling `MtpDevice.Connect()`.
For `PENTAX K-1 Mark II`, the model setter selects model number 78400, vendor
extension version 1, dual-slot/new-transfer behavior, old focus control, and
movie support.

`Connect()` then performs, in order:

1. WPD `IPortableDevice.Open`;
2. `VendorMode=true`, which sends vendor opcode `0x9001` with parameters
   `[13, 78400, 1, 1, 0]`;
3. `GetDeviceInfo` (`0x1001`);
4. `GetStorageIDs` (`0x1004`);
5. `GetAllConditions` (`0x900f`).

After `Connect()` returns, `MainWindow` installs property-change handlers and
calls `TimerInitialize()`. That creates a conditions timer whose first poll is
scheduled after 100 ms. All MTP commands share `_wpdCmdLock`, so timer and UI
commands serialize rather than overlap.

libgphoto2 matches steps 2–4 after opening its explicit PTP session, refreshes
DeviceInfo, and has now performed step 5 before setting. It additionally reads
object handles while initializing its filesystem cache. No IT2 source evidence
says object enumeration enables or disables setting control.

## UI-to-wire paths

The UI calls the selected-index property directly; there is no intermediate
commit command:

| Control | Selected-index setter | Property | Payload source |
|---|---|---:|---|
| shutter | `TvCurrentIndex` | `0xd00f` | `BitConverter.GetBytes(_TvList[index])`, 8-byte little-endian UINT64 |
| aperture | `AvCurrentIndex` | `0x5007` | `BitConverter.GetBytes(_AvList[index])`, 2-byte little-endian UINT16 |
| extended ISO | `SvCurrentIndex` | `0xd01e` | `BitConverter.GetBytes(_SvListEx[index])`, 4-byte little-endian UINT32 |
| exposure compensation | `XvCurrentIndex` | `0x5010` | `BitConverter.GetBytes(_XvList[index])`, 2-byte little-endian INT16 |

Each setter calls `MtpSetDevicePropValue`, which invokes
`ExecuteCommandWithDataToWrite(0x1016, [property], payload)`.

The WPD write implementation performs the ordinary three PTP phases through
Microsoft's MTP extension API:

1. begin operation `0x1016`, one UINT32 property parameter, and exact payload
   length;
2. write the payload bytes and confirm the host-side byte count;
3. end transfer and retrieve the PTP response code/parameters.

This is semantically the same command/data/response transaction produced by
`ptp_setdevicepropvalue`. Hardware traces confirmed our shutter payload
`010000007d000000` for 1/125 and ISO uses the same four-byte UINT32 format as
IT2. There is no checksum, framing suffix, vendor write opcode, reset, Green
button, capture-initiate, or property-specific commit in the IT2 call chain.

## Descriptor and condition handling

IT2 obtains the writable lists directly with `GetDevicePropDesc (0x1014)` even
when properties are absent from DeviceInfo. For K-1 II it prefers extended ISO
`0xd01e`, falling back to standard ISO `0x500f` only if the descriptor request
does not return PTP OK.

The conditions word at offset 504 gates UI visibility:

- bit 0: aperture changeable;
- bit 1: shutter changeable;
- bit 2: ISO changeable;
- bit 3: exposure compensation changeable;
- bit 5: task changing.

The tested K-1 II returned `0x0000000f`: all four exposure controls changeable,
not task-changing. Raw exposure mode 8 is IT2 Manual. Operation-inhibition UI
logic additionally blocks interval modes, drive modes 15/22, noise reduction,
and program mode 33; none is indicated by the captured idle conditions.

## Error handling finding

The four exposure selected-index setters collect the response code in a local
variable and do not inspect it. They do not read the descriptor back, raise
`CameraSettingFailed`, retry, delay, or restore. The next periodic conditions
poll refreshes current values; `MainWindow` then resets a combo box to the
camera-reported index when the user is no longer editing it.

Therefore IT2 source proves how to transmit a setting, but it does **not** prove
that an acknowledged request was applied. libgphoto2's post-write verification
is deliberately stricter and correctly detected the K-1 II retaining shutter
1/500 and ISO 200.

## Remaining differential and next experiment

Our bytes, property codes, model handshake, DeviceInfo refresh, storage query,
conditions gate, and idle state match IT2. Two differences remain credible:

1. IT2 is a long-lived session. Its first 100 ms timer poll begins after initial
   conditions loading, and it stays in vendor mode after a UI write. Our CLI
   setting session preflights once, verifies immediately, then exits.
2. IT2 uses the Windows WPD MTP extension transport, while libgphoto2 uses USB
   PTP containers directly. Both represent the same PTP phases, but only an
   official-client USB capture can rule out WPD/driver-added traffic.

The next hardware experiment must isolate difference 1 before blaming WPD:

1. open one exact-model session and perform the normal vendor/DeviceInfo/storage
   initialization;
2. read conditions, wait 100 ms, read conditions again;
3. confirm idle and the relevant changeability bit;
4. send one already-approved advertised ISO 200→400 request;
5. keep vendor mode active and poll conditions plus the `0xd01e` descriptor at
   100 ms intervals for at most 500 ms;
6. if 400 appears, immediately restore 200 in the same session and verify with
   the same bound; if it does not appear, report failure and send no other
   exposure write;
7. independently verify ISO 200 in a fresh session before exit from the gate.

This test needs a purpose-built harness so cleanup/restoration runs on every
branch. Do not approximate it with chained CLI processes. If it still retains
200, the next evidence should be a USB trace from IT2 on Windows, not speculative
opcodes or additional property writes.
