# K-3 III current-main direct-PC test — 2026-11-17

## Provenance

- Repository: `ian-morgan99/libgphoto2`
- Source SHA: `4ef07e9209e827a3ab0013edd5ac1f874892c423`
- Build: isolated Meson build with `-Dpentax_research_capture=true`, `-Dcamlibs=ptp2,directory`, and `-Diolibs=disk,libusb1`
- Build directory: `/tmp/libgphoto2-k3-current.lzR53V/build`
- `ptp2.so` SHA-256: `8145816290829ec22a7dbfbf18ea5abc0917c01e5f269f40bfa9f6bce06ffb9d`
- Camera: PENTAX K-3 Mark III, USB `25fb:0189`, firmware `2.20`
- Port: `usb:002,004`
- Host suite: `26/26` non-hardware tests passed

## Results

### Startup and recovery checks

- Initial exact-model init: PASS; vendor enable returned success, flags `0x00000000`.
- After the shutter test: exact-model init PASS and one-frame preview PASS.
- Five independent read-only configuration sessions: all PASS.
- Every read-only session reported `exposure-mode-raw=8` (`Manual`), `Tv-changeable=yes`, `shooting=no`, `processing=no`, `task-changing=no`, and `bulb-timer=no`.

### Preview

- Two-frame preview smoke, one post-failure frame and a 50-frame soak passed the harness's in-memory checks (JPEG MIME type and SOI/EOI bytes) and reported successful cleanup. The harness did not decode these images.
- No still capture, file publication, focus drive, or firmware operation was sent.

### Shutter round trip

The existing fail-closed harness read the live baseline as `00:30` and conditions as `30/1`, selected the advertised `1/16000` value, and attempted one target write:

```text
stage=initial widget=00:30 conditions_shutter=30/1
stage=target widget=1/16000 set_result=-1 conditions_shutter=30/1
stage=restore widget=00:30 set_result=0 conditions_shutter=30/1
```

Verdict: **target failed; original value independently confirmed in a later session.** The exact-baseline request returned success, but the target was never observed to apply, so this does not prove that a changed value can be restored. The generic `-1` could come from the SET or its verification. An earlier session applied `1/8000` and failed restoration. No new target write followed.

### Read-only advertised domains

- Shutter: current `00:30`; 58 advertised choices, including `1/16000` and `1/8000`.
- ISO: current `100`; 15 advertised choices.
- Aperture: current `f/2.8`; 19 advertised choices.
- EV: current `0`; 31 advertised choices.
- WB: current `auto`; 18 advertised choices.
- Colour temperature query returned `-1` in this session. The trace does not identify the failing check or PTP operation.

## Safety boundary

The current source and camera remain usable for read-only tests. No further setting-write or capture mutation is qualified from this session. A physical K-3 III power-cycle is required before another shutter A/B, because the first target write failed and the earlier session produced an unresolved restoration/vendor-latch failure. The next mutation must capture a raw `GP_LOG_ALL` trace around the first failed `0xd00f` transaction and must stop on the first mismatch.

## Retained local transcripts

The complete command transcripts remain in `/tmp/k3-current-session-20261117/`:

```text
01-init.log                 ff5a54e49715283605f00e88bfecab07c5fa5aa12914addc42336af54d0a04ca
02-preview-2.log            bb0f030cfe3030f3595a906dbada41da4805ba49eed4c856282f094fd33fde8e
03-shutter-roundtrip.log    9117757192a5f39551fd31469bce0fcde8473c06615b79473d5f8f800725f020
04-post-fail-init.log       ff5a54e49715283605f00e88bfecab07c5fa5aa12914addc42336af54d0a04ca
05-post-fail-preview.log    d42085fe1618c877d91a3751ba636aca0c2d6f497c9fe9dfa74f3532bf2adbfb
06-conditions-after-fail.txt fc170bc849f3765f5efe74ee1b1b22309ac85be916a7d1bb9e49127c0745c239
08-config-readonly.txt      c9faa7e9190e5441a3077e9297dac637791dd7a0d76147930160c09fa156c4c6
09-preview-50.log           5330145a1966bd191fbe03c05ae13cd838dfe6383591bc00e3b61cb44027ac6e
10-config-repeat.log        6472c9d068c80c4606d0d704e20baa6ba436522e18da260159bdb64f5c9690c3
```
