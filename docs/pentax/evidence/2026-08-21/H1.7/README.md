# H1.7 — K-1 Mark II passive identity and read-only status

Status: **INITIAL READ-ONLY PASS**

Camera: PENTAX K-1 Mark II in PC-P USB mode. Serial data is omitted.
Firmware: 1.02, returned by a separate read-only device-version query.

## Passive identity

- USB identity: `25fb:0183`.
- Interface class/subclass/protocol: `06/01/01` (Imaging/PTP).
- USB product string: `PENTAX K-1 Mark II`.
- Device node: `/dev/bus/usb/001/014` for this session only.
- No process owned the device node before either camera session.

This is the first real-hardware confirmation of the `0183` identifier. The
value was previously supported only by the extracted Windows-client mapping.
The abilities audit confirmed that only ptp2 claimed this model.

## Guarded lifecycle

One exact-model session read only `/main/status/cameramodel`. Vendor enable
returned success with function flags `0x00000003`; the read-only value was
`PENTAX K-1 Mark II`. This differs from the K-3 Mark III's observed flags 0 and
must remain a recorded model difference rather than being normalized away.

## GetAllConditions

A separate session read only `/main/status/pentaxconditions`. The bounded parser
accepted a 568-byte response and reported:

- operation state 0; no shooting or processing activity;
- raw exposure mode 21, raw user mode 0, raw drive mode 4;
- ISO 200, exposure step 1, `openAvNum=14`;
- Bulb timer disabled, raw timer rational 16/10;
- AstroTracer3, Astro shift, movement-failed, and time-too-long flags clear;
- optional Astro limit present with value 0; GPS state 0.

The widget was read-only and its implementation performs one `0x900f` call.
No property write, focus, preview, capture, transfer, delete, reset, or firmware
operation was issued. After each session the USB node was unowned.

## Limits and next gate

This proves initial identity, lifecycle, response length compatibility, and one
idle status parse only. It does not validate repeated cold/warm lifecycle,
visible setting correlations, live view, focus, Bulb/Astro behavior, capture,
transfer, or any writable property. The operator reported that the K-1 II does
not display its settings while connected in PC-P mode, so ISO 200 and the raw
mode values above are transport observations, not visual correlations. A later
correlation procedure must record settings before connection, reconnect without
changing them, read once, disconnect cleanly, and verify the same settings after
the camera returns to its normal display.

## Manual reference image and generic download

The operator disconnected USB, confirmed ISO 200, took one manual photograph
onto an SD card, and reconnected in PC-P mode. Read-only storage enumeration
found the new date folder `/store_00010001/DCIM/460_0821` and its sole file
`K1II8664.DNG`. One standard object download produced:

- size 38,468,076 bytes;
- SHA-256 `db9ea0c3f402fe6b8d5ad03ca8bcce7d3249ffe073fed7776401d44c1dbdb1a5`;
- EXIF model `PENTAX K-1 Mark II`;
- Program AE / Auto exposure, ISO 200, 6 seconds, f/1.4;
- `HD PENTAX-D FA* 50mm F1.4 SDM AW`, focal length 50 mm;
- capture time `2026:08:21 12:20:01`.

The temporary host copy was deleted after hashing/metadata extraction. The card
object was not modified or deleted, and USB was unowned afterward. ISO 200 now
has display, `GetAllConditions`, and EXIF agreement. Raw exposure mode 21 is a
strong Program AE candidate, but remains provisional because the status read
and photograph were separated by a disconnect rather than captured atomically.

The first selection attempt incorrectly assumed the highest-numbered filename
was newest and downloaded an older K-3 Mark III DNG. Its metadata exposed the
mistake; the temporary copy was deleted and the card was unchanged. The
correct rule is to enumerate DCIM folders and select the operator-created date
folder/object, never infer recency from a cross-camera sequence number.
