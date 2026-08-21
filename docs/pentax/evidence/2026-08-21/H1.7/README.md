# H1.7 — K-1 Mark II passive identity and read-only status

Status: **INITIAL READ-ONLY PASS**

Camera: PENTAX K-1 Mark II in PC-P USB mode. Serial data is omitted.

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
transfer, or any writable property. Before another K-1 II probe, record the
physical mode dial and visible ISO, then repeat this status read once for
correlation without changing a setting.
