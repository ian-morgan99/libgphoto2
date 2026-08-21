# P2–P5 implementation record — 2026-08-21

Status: **PASS for compilation; BLOCKED for hardware acceptance**

Source base: `9ca5375950853cc5ccc348b5b96ce6da925f3b79`, with the
working-tree Pentax implementation described below.

## Implemented

- guarded K-3 Mark III and K-1 Mark II identity mapping;
- per-session vendor state and vendor-mode enable/disable;
- host-side PTP wrappers with response/data length validation;
- live-view start, bounded JPEG extraction, and stop during exit;
- still-capture completion polling and cancellable transfer state machine;
- bounded filename, command, seek, allocation, block-size, and runtime handling;
- object acknowledgement only after successful local caching.

## Build command

The source was mounted read-only in intent (Docker volume) and built out of tree
at `/tmp/build` in an ephemeral Ubuntu 24.04 container with:

```sh
meson setup /tmp/build -Dcamlibs=ptp2 -Diolibs=libusb1
meson compile -C /tmp/build
```

Actual result: `camlibs/ptp2/ptp2.so` linked successfully. The project emitted
pre-existing warnings in unrelated code; no compiler error was emitted for the
Pentax changes.

After extracting the pure Pentax parsing and seekable transfer-buffer code, a
fresh container build and focused test run completed successfully:

```text
meson test -C /tmp/build test-pentax-utils --print-errorlogs
1/1 libgphoto2:test-pentax-utils OK
```

The fixture covers little-endian decoding; valid, truncated, oversized,
non-ASCII, and path-injection filenames; sequential, overwrite, and sparse
writes; begin/current/end seeking; negative bounds; invalid commands; and null
arguments.

A targeted earlier test run passed 6 of 9 tests. The three failures were from
the intentionally restricted container/build selection: host USB descriptor
access in `test-gp-port`, a model expected from a non-PTP camlib in
`test-gphoto2`, and `test-filesys` under that restricted setup. These are not
recorded as regressions or as passes; a full-build test run remains required.

## Blocker

No Pentax camera was connected. Consequently handshake, preview, capture,
transfer, cancellation, reconnect, and configuration acceptance are BLOCKED.
No artifact hashes or hexadecimal camera response codes exist yet.
