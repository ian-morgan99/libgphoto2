# Pentax capability and verification matrix

This file records what the current driver advertises and the evidence needed to
claim that it works. It is not a transcription of every feature visible in a
proprietary client. The earlier 621-line extraction mixed inferred model flags,
uncertain USB IDs, and unimplemented properties; it is archived outside the
repository in workspace
`archive/CameraCapabilities.extracted-obsolete-20260821.md`.

The canonical implementation plan is `DEVELOPMENT_PLAN.md`. Wire details are in
`docs/pentax/PENTAX_WIRE_PROTOCOL.md`; configuration coverage is in
`docs/pentax/PENTAX_CONFIGURATION.md`.

## Guarded vendor-mode models

Vendor commands are enabled only when all three identity fields match:

| Abilities VID:PID | Exact DeviceInfo model | Vendor model | Extension | Evidence |
|---|---|---:|---:|---|
| `25fb:018c` | `PENTAX K-3 Mark III` | 78420 | 1 | Observed client; hardware blocked |
| `25fb:0183` | `PENTAX K-1 Mark II` | 78400 | 1 | Observed client; hardware blocked |

`25fb:018f` is the K-3 Mark III Monochrome entry and is deliberately excluded
from this vendor implementation. All unmatched identities remain on generic
PTP and receive no Pentax vendor operations.

## Acceptance status

“Implemented” means compiled and covered by host-side parser/state tests. It
does not mean camera-verified.

| Capability | K-3 III | K-1 II | Current evidence | Hardware acceptance gate |
|---|---|---|---|---|
| Generic PTP files | Existing | Existing | Existing ptp2 path compiles | enumerate/download/hash |
| Vendor lifecycle | Implemented | Implemented | exact identity and response guards tested | enable, disable, reconnect |
| Live-view JPEG | Implemented | Implemented | bounded parser and restore paths compile | 500 frames, stop, reconnect |
| Capture/download | Implemented | Implemented | fault-injected transfer grammar passes sanitizers | JPEG and RAW hash checks |
| Cancellation/timeout | Implemented | Implemented | callback fault tests pass | cancel at each phase, reconnect |
| ISO | Generic descriptor path | Generic descriptor path | `0x500f`, `UINT16` | read and round-trip supported value |
| Aperture | Generic descriptor path | Generic descriptor path | `0x5007`, `UINT16` | read and round-trip supported value |
| White balance | Generic descriptor path | Generic descriptor path | `0x5005`, `UINT16` | read and round-trip supported value |
| Focus mode | Generic descriptor path | Generic descriptor path | `0x500a`, `UINT16` | read and round-trip supported value |
| Exposure compensation | Generic descriptor path | Generic descriptor path | `0x5010`, `INT16` | read and round-trip supported value |
| Shutter speed | Existing Pentax/Ricoh path | Existing Pentax/Ricoh path | `0xd00f`, `UINT64` | descriptor/type/value round-trip |
| Polaris packaging | Source path implemented | Source path implemented | provenance, safety, ARM loader builds | FwPkt QEMU/repack/install/revert |

## Explicitly not claimed

Movie transfer, composition adjustment, Astrotracer, dust reduction, pixel
shift, detailed custom-image controls, K-3 III Monochrome, KP, 645-series, GR,
and G900SE vendor control are outside the accepted scope. Their observed names
or opcodes are not sufficient evidence to implement them.

No supported-camera claim may replace a blocked cell until the corresponding
hardware record, logs, hashes, and recovery result are committed as specified by
`DEVELOPMENT_PLAN.md`.
