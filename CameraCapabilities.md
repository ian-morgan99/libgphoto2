# Pentax capability and verification matrix

This file records what the current driver advertises and the evidence needed to
claim that it works. It is not a transcription of every feature visible in a
proprietary client. The earlier 621-line extraction mixed inferred model flags,
uncertain USB IDs, and unimplemented properties; it is archived outside the
repository in workspace
`archive/CameraCapabilities.extracted-obsolete-20260821.md`.

The canonical implementation plan is `DEVELOPMENT_PLAN.md`. The exhaustive
normative target is
[`docs/pentax/IMAGE_TRANSMITTER_CAPABILITY_MATRIX.md`](docs/pentax/IMAGE_TRANSMITTER_CAPABILITY_MATRIX.md).
Wire details are in `docs/pentax/PENTAX_WIRE_PROTOCOL.md`; configuration
coverage is in `docs/pentax/PENTAX_CONFIGURATION.md`. This summary must not
substitute for the normative matrix.

## Guarded vendor-mode models

Vendor commands are enabled only when all three identity fields match:

| Abilities VID:PID | Exact DeviceInfo model | Vendor model | Extension | Evidence |
|---|---|---:|---:|---|
| `25fb:0189` | `PENTAX K-3 Mark III` | 78420 | 1 | Hardware USB/DeviceInfo verified on firmware 2.20 |
| `25fb:0183` | `PENTAX K-1 Mark II` | 78400 | 1 | Hardware-confirmed PC-P identity; initial read-only lifecycle pass |

The K-3 III exposes Microsoft MTP VendorExtensionID `0x6`; exact USB and
DeviceInfo identity, rather than that generic extension ID, gates Pentax vendor
mode. `25fb:018a` is its CD-ROM installer mode and is excluded.

`25fb:018f` is the K-3 Mark III Monochrome entry and is deliberately excluded
from this vendor implementation. All unmatched identities remain on generic
PTP and receive no Pentax vendor operations.

## Acceptance status

“Implemented” means compiled and covered by host-side parser/state tests. It
does not mean camera-verified.

| Capability | K-3 III | K-1 II | Current evidence | Hardware acceptance gate |
|---|---|---|---|---|
| Generic PTP files | Existing | DNG download pass | K-1 II 38,468,076-byte DNG hashed and EXIF parsed without deletion | add JPEG and repeat hashes on both bodies |
| Vendor lifecycle | Initial 3/3 controlled cold and 3/3 warm pass; larger gates open | Initial read-only pass, flags `0x3` | exact identity/response guards; earlier poorly isolated K-3 III runs returned `0x2002` | complete per-body cold/warm gates |
| Live-view JPEG | 50-frame bounded warm-session pass | Implemented, hardware blocked | bounded parser and restore paths compile | first pass cold lifecycle, then 500 frames, stop, reconnect |
| Capture/download | Research code, default-disabled | Research code, default-disabled | fault-injected grammar passes sanitizers; ability and dispatch containment enforced | explicit research build only, then JPEG and RAW hash checks |
| Cancellation/timeout | Implemented | Implemented | callback fault tests pass | cancel at each phase, reconnect |
| ISO | Vendor `0xd01e`, public setter withheld | Image Transmitter maps vendor `0xd01e`; hardware read pending | K-3 III display-confirmed 3200→1600→3200 round trip; K-1 II B snapshots report ISO 200 through conditions | read the source-identified K-1 II descriptor before any write |
| Aperture | Generic descriptor path | Image Transmitter maps `0x5007` | `UINT16`; not advertised by the tested K-1 II DeviceInfo path | direct read, then round-trip a supported value |
| White balance | Generic descriptor path | Generic descriptor path | `0x5005`, `UINT16` | read and round-trip supported value |
| Focus mode | Absent in observed vendor descriptor set | Generic descriptor path | `0x500a` absent on K-3 III firmware 2.20 | identify correct property before any write |
| Exposure compensation | Generic descriptor path | Image Transmitter maps `0x5010` | `INT16`; not advertised by the tested K-1 II DeviceInfo path | direct read, then round-trip a supported value |
| Shutter speed | Existing Pentax/Ricoh path | Image Transmitter maps vendor `0xd00f` | K-3 III `UINT64`; K-1 II conditions report a 30-second Bulb timer | direct K-1 II descriptor read before any write |
| Polaris packaging | Source path implemented | Source path implemented | provenance, safety, ARM loader builds | FwPkt QEMU/repack/install/revert |

## Explicitly not claimed

Movie transfer, composition adjustment, Astrotracer, dust reduction, pixel
shift, detailed custom-image controls, K-3 III Monochrome, KP, 645-series, GR,
and G900SE vendor control are outside the accepted scope. Their observed names
or opcodes are not sufficient evidence to implement them.

No supported-camera claim may replace a blocked cell until the corresponding
hardware record, logs, hashes, and recovery result are committed as specified by
`DEVELOPMENT_PLAN.md`.
