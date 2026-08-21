# H1.10 — K-1 Mark II live-view control state

Camera: PENTAX K-1 Mark II firmware 1.02 in PC-P mode. Serial omitted.

## Safety envelope

The diagnostic was compiled in an isolated container. Focused parser and camera
list tests passed before hardware access. Each invocation selected the exact
model and current USB node, ran under a 40-second timeout, and the container
could access only that USB node. No property setter, live-view frame, focus,
capture, transfer, delete, reset, shutdown, or firmware operation was used.

Two preparatory sessions failed safely:

1. An invalid CLI configuration path ended after initialization and before the
   three property reads.
2. A generic PTP array decoder read `0xd009` but rejected the compound Pentax
   payload locally. No later property and no write was attempted. The code was
   replaced with a bounded raw-data getter and rebuilt before continuing.

All sessions completed vendor disable/session cleanup, and the USB node was
unowned afterward.

## Successful read-only result

One named `pentaxliveviewcontrols` read performed, in order:

1. `GetDevicePropValue(0xd009)` — live-view/CAF geometry;
2. `GetDevicePropValue(0xd036)` — live-view AF position; and
3. `GetDevicePropValue(0xd037)` — live-view zoom state.

The validated values were:

- live-view area: 720×480;
- active area: 720×480;
- contrast-AF active area: 576×320;
- contrast-AF spot: 48×48;
- AF position: 360,240 (the exact live-view-area centre);
- AF-position response length: 8 bytes; and
- zoom raw value: 1 (IT2's disabled/no-zoom value).

Status: **READ-ONLY PASS**. This closes the K-1 II discovery half of the
`0xd009`/`0xd036`/`0xd037` gate. It does not prove that AF position or zoom is
writable, does not start PC live view, and does not authorize either setter.
