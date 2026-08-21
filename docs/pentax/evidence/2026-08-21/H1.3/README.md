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

A subsequent purpose-built in-memory run produced 10/10 complete JPEGs with a
250 ms inter-frame delay and successful explicit camera exit. An initial
unpaced run produced two valid frames and then a frame-3 error; this is retained
as evidence that callers must pace preview requests. Neither run retained image
data. The 500-frame gate remains open.

After correcting exit error propagation so a live-view restoration failure
cannot be masked by successful vendor-mode cleanup, the same harness produced
50/50 complete JPEGs with 250 ms pacing. The strengthened camera exit returned
success. No frame was retained and no shutter, card, setter, delete, reset, or
firmware operation was issued. This is a bounded soak result, not the canonical
500-frame gate.

For controlled repetition, build and run the purpose-built in-memory harness:

```sh
CAMLIBS=/path/to/build/camlibs/ptp2 \
IOLIBS=/path/to/build/libgphoto2_port/libusb1 \
LD_LIBRARY_PATH=/path/to/build/libgphoto2:/path/to/build/libgphoto2_port/libgphoto2_port \
/path/to/build/examples/pentax-safe-preview usb:BUS,DEVICE 10
```

The frame count is strictly limited to 1 through 500. The tool selects only the
exact K-3 III MTP ability, retains no frame, and has no configuration setter,
still-capture, delete, reset, or firmware call site.
