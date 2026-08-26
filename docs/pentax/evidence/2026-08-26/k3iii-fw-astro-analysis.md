# K-3 III firmware (fwdc233b.bin, v2.20) — Astro-mode capture analysis

Date: 2026-08-26
Firmware: /home/ian/Downloads/k3III_v220/fwdc233b.bin (35,106,716 bytes)
Header: `PENTAX\0K-3 Mark III\0RICOH IMAGING COMPANY, LTD.\0`, size fields at 0x30/0x34.

## Goal
Identify how the camera captures a photo in Astro exposure mode (ExpMode 20)
with the mode dial at B, since vendor InitiateCapture 0x9011 is rejected with
0x2002 in that mode on the wire (see k3iii-bulb5s-retest.log).

## Findings

### 1. Supported vendor operation table (DeviceInfo blob @ file offset 0x1f4f8dc)
Ascending u16-LE list of every PTP op the K-3 III firmware implements:

    0x9001-0x9006, 0x900b-0x900d, 0x900f-0x9011,
    0x9014-0x9017, 0x9019-0x9024,
    0x902d-0x902f, 0x9031, 0x9032, 0x9034-0x9036, 0x9038,
    0x903c-0x903f, 0x9041, 0x9042, 0x9044, 0x9047, 0x9049-0x904b,
    0x904d-0x9055, 0x9058, 0x9059, 0x905b-0x905e, 0x9060, 0x9062,
    0x9063, 0x9067-0x9069, 0x906b, 0x906d-0x9070, 0x9072-0x907f

Ops beyond our current ptp.h defines: 0x9002-0x9005, 0x9015, 0x901a-0x9024,
and everything from 0x902d up. The astro-specific capture path, if it is a
vendor opcode at all, must live in one of these undocumented ops.

### 2. Device property tables (@ 0x1e00c1a onward)
Five per-model property blocks (0xd0xx/0xd1xx/0xd2xx/0xd3xx/0xd4xx ranges),
~95 props each. These are template blobs; live enumeration via
GetDevicePropDesc is the reliable way to identify astro props.

### 3. String mining
Readable strings survive only as LZ literals between compressed spans:
- `AstroTr` @ 0x853b27 (AstroTracer UI resource)
- `M6AstroT…` @ 0x909a94 (mode-6 astro symbol fragment, near `ExposureData`)
- `AstroCalcFwhm` @ 0xa1ab14 (FWHM calc for astrotracer alignment)
- `A:\Resource\Jpeg\Astro.jpg`, `AstroMo.jpg` (menu icons)
- No `StarAf*` / `ReadAstro` / `AstroPhotoParam` strings (those are K-1 II
  fw-only names). Star AF trigger has NO identifiable vendor opcode in this
  firmware either — consistent with the "not implemented" audit entry.

### 4. Compression blocks static disassembly
Entropy scan: alternating CODE/DATA (~7.1-7.9) and COMP (~8.0) bands from
0x6c0000 up; no standard container magic (not lzma/xz/gzip/zlib). Same custom
packing as K-1 II fwdc240b.bin. u16 scans for 0x9011 produce only false
positives in numeric data; handler code is inside compressed bands.

## Conclusion
Static analysis cannot reveal the astro capture path without decompressing
the packed body. The actionable next step is live probing: once the camera
exits its busy state (state=49), enumerate ops 0x901a-0x9024 and d0xx-d4xx
props on hardware while in Astro mode, and test standard PTP 0x100E
InitiateCapture as an alternative to vendor 0x9011.
