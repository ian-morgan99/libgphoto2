# H1.3 — single live-view frame

Camera: K-3 Mark III firmware 2.20. Serial and image content are not retained.

## Safety envelope

One candidate `capture-preview` operation was run inside a disposable container
with an outer timeout. The driver read the original USB live-view property,
temporarily enabled live view, requested one `0x9006` frame, restored the exact
original property, and disabled vendor mode. No shutter release, capture,
device-property configuration, card write, file delete, reset, or firmware
operation was requested.

## Results

- JPEG validation: baseline, 8-bit, three components.
- dimensions: 1080 by 720.
- returned size: 84,488 bytes.
- a separate post-preview candidate summary reported `0xd035` current value 0.
- the image existed only inside the disposable container and was not retained.

This passes a one-frame vertical slice only. It does not pass the canonical
500-frame preview stress gate.
