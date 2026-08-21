# H1.8 — K-1 Mark II B-mode read-only baseline

Camera: K-1 Mark II firmware 1.02, physical mode dial `B`. Serial omitted.

## Safety envelope

The current source commit was compiled in a fresh isolated container. The
container could access only the current camera USB node; the exact model and
port were supplied on every invocation. One named `pentaxconditions` read was
followed by two independent complete public configuration reads. Every session
had a 30- or 40-second host timeout and exited normally without retry.

The exact logs for each full configuration read contained:

- vendor enable and disable (`0x9001`) only;
- standard discovery/session operations;
- five read-only `GetDevicePropDesc` (`0x1014`) operations;
- one `GetAllConditions` (`0x900f`) operation; and
- zero property setters, focus, preview, capture, transfer, delete, reset,
  shutdown, or firmware operations.

USB was unowned after the reads. Raw logs and camera serial data were not
committed.

## Results

The named read and both configuration reads returned a 568-byte condition
structure with the same decoded values:

- state 0 and no shooting, processing, or task-changing activity;
- exposure mode raw 9 and drive mode raw 0;
- ISO 200 and `openAvNum=14`;
- Bulb timer enabled with duration 30/1 seconds;
- Tv changeability false; and
- AstroTracer3, Astro shift, movement-failed, time-too-long, and GPS flags
  clear, with Astro limit 0.

Both public configuration snapshots contained 188 lines. After removing only
the advancing camera clock and battery value, their SHA-256 was identically
`aefcf1ccf4de83256f375b1942d5ea08e4bb8d0469b62a456c735d8677babdfd`.
The body advertised only five generic device-property descriptors in this
vendor session. This is not a capability boundary. Image Transmitter 2 handles
the K-1 II explicitly as model number 78400 with vendor extension 1, dual slots,
new transfer mode, and movie support. It directly requests descriptors that are
not present in DeviceInfo, including shutter `0xd00f`, aperture `0x5007`, ISO
`0xd01e`, exposure compensation `0x5010`, and focal length `0x5008`.

The correct next step is therefore a complete source-derived K-1 II capability
table followed by individually guarded direct descriptor reads. Absence from
DeviceInfo must be recorded separately from absence of official-client support.

Status: **READ-ONLY PASS (2/2)**. This correlates raw exposure mode 9 and the
30-second Bulb-timer status with the operator-selected B mode. It does not test
changing the timer, starting an exposure, Astro behavior, focus, live view, or
capture.
