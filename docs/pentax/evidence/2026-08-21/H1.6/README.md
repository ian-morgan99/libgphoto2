# H1.6 — Bulb-mode read-only configuration baseline

Camera: K-3 Mark III firmware 2.20, physical mode dial `B`. Serial omitted.

## Safety envelope

Two independent complete public configuration snapshots were read using the
corrected container isolation: USB-bus visibility plus an explicit Docker grant
for only the current camera node. Both snapshots contained 700 lines and
returned success. The exact protocol audit on snapshot 2 contained zero
transmitted `SetDevicePropValue`, focus, or capture commands. No exposure,
preview, file, delete, reset, shutdown, or firmware operation was requested.

## Bulb-specific results

- Public shutter and raw `0xd00f` expose 46 choices from 1 through 600 seconds.
- Current shutter is 300 seconds.
- Raw current `0xd00f` is decimal `4294967596`, hexadecimal
  `0x000000010000012c`: denominator 1 and numerator 300 in the existing
  fractional-time representation.
- `0xd013`, source-correlated with Image Transmitter's byte-valued
  `DriveModeNew`, reports current 0 and only three choices in B. The M baseline
  reported current 4 and twelve choices.
- `0xd01e` remains 3200 with the same 15-value ISO domain observed in M.
- Aperture remains f/3.5 on the attached 24–70 mm lens.
- Other enumerated `0xd0xx` property currents and choice counts match the M
  summary except for the explicitly listed shutter and drive-mode changes.

The three B-mode `0xd013` values are not labelled yet. Image Transmitter proves
the property association and UINT8 type, but its UI uses image/index mappings
rather than a stable protocol enum table. Camera-display correlation must
precede labels or writes.

Status: **READ-ONLY PASS (2/2)**. This proves stable B-mode enumeration and a
distinct 1–600 second timer domain. It does not prove that setting a timer or
starting/ending a Bulb exposure is safe or correct.
