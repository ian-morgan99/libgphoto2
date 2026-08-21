# Pentax real-hardware test log

This is the canonical, chronological record of tests executed against a
physical Pentax camera. It is separate from `DEVELOPMENT_PLAN.md`, which tracks
work and acceptance gates, and from the smaller per-gate evidence records.

Camera: PENTAX K-3 Mark III, firmware 2.20. Test date: 2026-08-21. The camera
serial number and all image content are deliberately omitted. Unless explicitly
stated otherwise, tests used the colour K-3 Mark III in MTP mode over USB.

## Non-destructive test policy

Every hardware test in this record was bounded and selected to avoid persistent
camera or card changes. Before direct USB access, GVFS was unmounted and the USB
device node was checked with `fuser`; a probe was aborted if another process
owned the device. Candidate builds ran from a read-only source mount in a
disposable Ubuntu container.

No completed test in this record issued a still-capture, shutter-release,
delete, file-write, reset, firmware-update, or persistent configuration command.
Preview JPEGs existed only in memory or disposable storage and were not retained.
Camera serial data seen during discovery was not committed.

## Chronological results

| Test | Operations allowed | Result | Code consequence |
|---|---|---|---|
| Passive enumeration in MTP mode | Linux USB descriptor reads only | `25fb:0189`; Imaging interface; 480 Mbit/s | Replaced speculative K-3 III PID `018c` with observed PID `0189` |
| Passive enumeration in CD-ROM mode | Linux USB descriptor reads only | `25fb:018a`; Mass Storage interface | Documented as installer mode, not the tethering transport |
| Generic MTP discovery | Forced generic PTP/MTP DeviceInfo and storage reads | Exact model `PENTAX K-3 Mark III`; firmware 2.20; Microsoft MTP extension `0x00000006`; roots SD1 and SD2 | Removed the assumption that Pentax routing could depend on a Pentax VendorExtensionID |
| Pre-vendor descriptor baseline | DeviceInfo/property reads | Only the standard MTP operation/property set was visible | Established a baseline for checking vendor-mode teardown |
| Guarded vendor-mode lifecycle | Pentax `SetVendorMode(enable)`, descriptor reads, then `SetVendorMode(disable)` | Enable exposed Pentax descriptors; a separate generic session after exit showed the original five properties | Routed the session using exact VID, PID, and DeviceInfo model; added post-enable DeviceInfo refresh and rollback |
| Read-only property descriptors | Descriptor/value reads only | WB `0x5005`, F-number `0x5007`, exposure compensation `0x5010`, shutter `0xd00f`, and USB live view `0xd035` were present; standard ISO `0x500f` and focus mode `0x500a` were absent | Unsupported properties remain withheld; unknown `0xd01e` semantics were not invented |
| Read-only configuration widgets | Candidate configuration reads only | WB, aperture, exposure compensation, and shutter widgets decoded; unmapped WB values remained explicitly unknown | Corrected effective-vendor routing for Pentax configuration tables |
| Single preview frame | Temporary live-view enable, one `0x9006` frame read, restore original live-view value, vendor-mode disable | Complete baseline JPEG, 1080x720, 84,488 bytes; independent post-check reported `0xd035=0` | Validated the first live-view vertical slice |
| Initial unpaced preview loop | Same temporary live-view operations; no retention | Frames 1 and 2 were valid; frame 3 returned an error; cleanup ran | Added conservative inter-frame pacing and precise failure-stage reporting |
| Paced preview loop | Ten in-memory previews at 250 ms spacing | 10/10 complete JPEGs; explicit camera exit succeeded | Established a safe repeatable harness |
| Paced preview soak after cleanup hardening | Fifty in-memory previews at 250 ms spacing | 50/50 complete JPEGs; strengthened camera exit reported cleanup success | Preserved live-view restoration errors instead of allowing later cleanup success to mask them |
| First cold camera restart and read-only config enumeration | Two full config-tree reads, followed by one named shutter read with sanitized lifecycle logging | Camera re-enumerated as `25fb:0189`, but vendor enable `0x9001` returned General Error `0x2002`; only the five generic MTP properties were exposed and no later Pentax opcode was sent | Treat vendor entry as state-dependent and fail closed; do not claim restart stability or vendor settings in this session |
| Delayed cold-session retry | Release USB ownership, wait five seconds, open a fresh session, and request the named shutter setting | Vendor enable again returned `0x2002`; shutter was absent because the driver remained safely generic | Startup timing is not a sufficient explanation; do not loop retries or continue vendor probes without identifying the missing state/prerequisite |
| Controlled clean power-on lifecycle, cycles 1–2 | Before each cycle confirm camera absent and all prior sessions terminated; user powered on to normal shooting screen; verify `25fb:0189` and exclusive ownership; make one fresh connection and read shutter descriptor | Both first `0x9001` enables succeeded with flags 0; `0x1014` read `0xd00f`; current shutter was 1 second with 55 enumerated choices from 1/8000 through 30 seconds; both clean exits sent `0x9001` disable and returned success | Controlled cold-start sequence passes 2/2; one more identical cycle is required for the initial 3/3 checkpoint, and earlier `0x2002` failures still require explanation |

The 50-frame result is a bounded soak, not the plan's 500-frame acceptance gate.
Still capture, transfer from camera storage, configuration writes, cancellation,
physical disconnect/reconnect, and the 500-frame gate remain untested on hardware.
One camera power-cycle/re-enumeration has now been observed, but it did not pass
vendor-mode entry and therefore does not satisfy the reconnect gate.

## USB identifier postmortem

### What was wrong

The initial `ptp2` candidate registered the colour K-3 Mark III as
`25fb:018c` and called the transport PTP mode. The attached camera proved that
its tethering setting is MTP and that it enumerates as `25fb:0189`. Its CD-ROM
setting enumerates separately as `25fb:018a`.

### Why it should have been caught immediately

The correct `0189` identifier was not difficult to discover. It was available
from one passive `lsusb` read as soon as the camera was attached. More
importantly, it was already in this repository: upstream commit `1c4f11313`
added K-3 III PID `0189` to `camlibs/pentax/library.c` in 2023. The unfinished
`ptp2` prototype separately contained a commented `018c` entry with no evidence
record. That commented value was promoted by commit `f5eebff6f` before hardware
was available.

The failure was therefore procedural, not a difficult reverse-engineering
problem:

1. The implementation treated a commented prototype value as an input fact.
2. The repository review did not reconcile duplicate model knowledge across
   `camlibs/pentax` and `camlibs/ptp2`.
3. The first plan allowed implementation before the passive USB identity gate.
4. The model test proved internal consistency with `018c`; it did not prove that
   `018c` matched a real device. A synthetic test cannot validate an identifier
   copied into both implementation and expectation.
5. The absent camera was recorded as a blocker, but the unverified identifier
   was still allowed to flow into feature code instead of remaining an explicit
   unknown.

There is no hardware evidence in this work that `018c` identifies this camera
in any mode. It must not be presented as an alternate K-3 III identifier.

### Corrective controls

- Hardware identity is now the tuple `25fb:0189` plus the exact DeviceInfo model
  `PENTAX K-3 Mark III`; VID/PID alone does not authorize vendor commands.
- The colour K-3 III `0189` registration was removed from the legacy USB-SCSI
  camlib so the same physical device is not claimed by two drivers.
- The ability name now says MTP mode.
- Passive VID/PID, interface class, exact DeviceInfo model, firmware, and
  VendorExtensionID discovery precedes every model-specific hardware gate.
- Conflicting identifiers found anywhere in the repository must be resolved
  against passive hardware evidence before implementation proceeds.
- Tests must distinguish repository consistency from real-hardware validation.

## Reproduction and evidence locations

The constrained preview harness is `examples/pentax-safe-preview.c`. It accepts
only the exact K-3 III MTP model, an explicit USB port, and 1 through 500 frames.
It has no configuration setter, still-capture, delete, reset, firmware, or image
retention path.

Detailed records are retained in:

- `docs/pentax/evidence/2026-08-21/H1.1/README.md` — passive discovery;
- `docs/pentax/evidence/2026-08-21/H1.2/README.md` — vendor lifecycle and descriptors;
- `docs/pentax/evidence/2026-08-21/H1.3/README.md` — live-view probes and soak.

Implementation commits arising directly from these tests are `b72d9cbdc` and
`abf55fbcb`.
