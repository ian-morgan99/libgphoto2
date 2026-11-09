# K-1 II PC live-view preview — bounded 5-frame run, wedged at frame 3

Date: 2026-11-09 (UTC). Operator reported the K-1 II reconnected to this PC.

## Build under test

- Source tree: `/home/ian/Documents/VSCodeProjects/LibGphoto2-v86-main`
- Commit: `7e6dec95f` (`origin/main`, "docs: state which repo owns which work, with the #145 worked example")
- Build directory: `build-k1-research` (meson option `pentax_research_capture=true`, so
  `LIBGPHOTO2_ENABLE_PENTAX_RESEARCH_CAPTURE=1`; Pentax PC-LV preview is compiled in)
- Targets rebuilt for this run: `camlibs/ptp2/ptp2.so`, `examples/pentax-safe-focus`,
  `examples/pentax-safe-preview`
- Full-tree `ninja` fails on unrelated camlibs (`ax203`, `docupen`: missing `gd.h`). Only the
  ptp2 and example targets above were built; that is sufficient for these probes.

## Device and environment

Pre-flight per `docs/pentax/HARDWARE_TESTING_RUNBOOK.md` §6:

- No `gvfsudisks`/`gphoto2`/`gphotofs` process was holding the port (`pgrep` empty).
- sysfs enumeration before the run:

```
FOUND: /sys/bus/usb/devices/1-4 vendor=25fb product=0183 bus=1 dev=5 model=PENTAX K-1 Mark II
```

- PID `0183` = PTP mode (§6.3). Not mass-storage `0182`, so testing was allowed to proceed.
- Port derived from sysfs, not hardcoded: `usb:001,005`.

```bash
B=$(pwd)/build-k1-research
export LD_LIBRARY_PATH=$B/libgphoto2:$B/libgphoto2_port/libgphoto2_port
export CAMLIBS=$B/camlibs/ptp2
export IOLIBS=$B/libgphoto2_port/libusb1:$B/libgphoto2_port/usbscsi
```

Note: the runbook §4.1 exports `IOLIBS`. An earlier attempt used `IO_LIBS`, which produced
`error=Error loading a library (-4)` because ltdl could not find `usb1.so`. The variable name
matters.

## Step 1 — read-only init probe (no focus command)

```
$ examples/pentax-safe-focus "Pentax K-1 Mark II (PTP mode)" usb:001,005 init
selected_model=Pentax K-1 Mark II (PTP mode) usb=25fb:0183 port=usb:001,005
Pentax session already open from a previous connection; observing camera state.
Pentax init stage vendor enable succeeded; function flags 0x00000003.
focus_init=command-accepted cleanup=ok retries=0
```

Exit 0. VID:PID verified as `25fb:0183` before any write, per §4.1.

Note the first attempt used the model spelling `PENTAX K-1 Mark II` and returned
`Unknown model (-105)`. The registered public spelling is `Pentax K-1 Mark II (PTP mode)`
(`camlibs/ptp2/library.c:2821`).

## Step 2 — bounded 5-frame preview

```
$ examples/pentax-safe-preview "Pentax K-1 Mark II (PTP mode)" usb:001,005 5
Pentax session already open from a previous connection; observing camera state.
Pentax init stage vendor enable succeeded; function flags 0x00000003.
Pentax preview stage start-PC-LV returned 0x2001.
Pentax preview stage get-frame returned 0x2001 (17093 bytes, 21 attempts, 673 ms).
frame=1 bytes=17093 valid_jpeg=yes
Pentax preview stage start-PC-LV returned 0x2001.
Pentax preview stage get-frame returned 0x2001 (28271 bytes, 9 attempts, 290 ms).
Preview captured, but restoring PC live view failed with 0x02ff.
frame=2 bytes=28271 valid_jpeg=yes
Pentax preview stage get-frame returned 0x02f9 (0 bytes, 1 attempts, 0 ms).
Pentax preview stage restore-after-frame returned 0x02f9.
frame=3 stage=capture-preview error=Could not find the requested device on the USB port (-52)
safe preview failed: Could not find the requested device on the USB port (-52)
```

Exit 1. Frames 1 and 2 delivered; frame 3 failed; the run aborted before frames 4 and 5.

## Post-run device state

Immediately after the failure, `25fb` is absent from the bus entirely:

```
$ grep -l 25fb /sys/bus/usb/devices/*/idVendor   ->  NONE
$ ls /sys/bus/usb/devices/                        ->  no 1-4 entry
```

No process of mine was holding the port (`pgrep` for `pentax-safe|gphoto2|gphotofs|gvfs` empty),
so this is not a host-side descriptor leak. This matches the documented wedge signature in
`HARDWARE_TESTING_RUNBOOK.md` §6.5, where a USB-level reset did **not** recover PTP and only a
physical power cycle did.

**No recovery was performed or claimed.** The camera is currently unreachable from this host.

## What this run does and does not prove

Proven:

- Vendor-mode enable works on this body/firmware (`function flags 0x00000003`).
- PC-LV start returns `0x2001` and at least two frames transfer with non-trivial sizes.

Not proven, deliberately:

- **`valid_jpeg=yes` is not evidence of frame integrity here.** This build is `origin/main`,
  which still contains the old first-`ff d9` byte-scan in `pentax_jpeg_bounds()`. The marker-walk
  replacement (`0b31c524e`) lives on `feature/polaris-capture-events` and is **not** on `main`.
  The old scan is the one that can be fooled by an EXIF thumbnail EOI, so these two frames must be
  re-validated against the new parser before any frame-integrity claim.
- No focus command was sent. Neither `0x9016` nor `0x9017` was issued, so this run contributes
  **nothing** to the unresolved old-focus/new-focus direction question in
  `FOCUS_LIVE_VIEW_MODEL_AUDIT.md`. No lens movement was observed or claimed.
- No still capture, no card write, no parameter write.

## Open defect surfaced

Frame 2's post-frame PC-LV restore returned `0x02ff` (invalid dataset / general operation error),
then frame 3's start returned `0x02f9` and the device left the bus. The sequence in
`camera_pentax_capture_preview` is: start PC-LV, get frame, restore state after frame. The restore
after a successful frame is where this body diverged from IT2's expected stop behaviour
(`PcLiveViewMode` accepts `0xa005` or `0x2001` on stop). Whether the `0x02ff` restore is the cause
of the disappearance or a symptom of an already-failing body is not determinable from one run and
is not claimed either way.

---

## Follow-up after operator power cycle — marker-walk build, 2 frames, clean

Same day. The operator power-cycled the body; it re-enumerated as
`/sys/bus/usb/devices/1-4`, `25fb:0183`, **devnum 6** (was 5).

Build under test:

- Tree `/home/ian/Documents/VSCodeProjects/LibGphoto2/libgphoto2`, branch
  `feature/polaris-capture-events` at `c676066b6`.
- Fresh meson tree `build-k1ii-marker` (`-Dcamlibs=ptp2 -Dpentax_research_capture=true`).
- This tree contains the marker-walk `pentax_jpeg_bounds()` (`0b31c524e`), which `main` does not.
- Built targets: `camlibs/ptp2/ptp2.so`, `examples/pentax-safe-preview`,
  `examples/pentax-safe-focus`, `libgphoto2_port/libusb1/usb1.so`,
  `tests/test-pentax-utils`.

Port-driver note: this tree does not build `usbscsi.so`, and ltdl could not resolve the driver
from `$B/libgphoto2_port/libusb1` directly (`error=Error loading a library (-4)`). Staged per
runbook §4.2 instead:

```bash
mkdir -p /tmp/iolibs-1109
cp $B/libgphoto2_port/libusb1/usb1.so /tmp/iolibs-1109/libusb1.so
export IOLIBS=/tmp/iolibs-1109
```

Parser regression test in this tree: `tests/test-pentax-utils` exit 0 (includes the EXIF
thumbnail case at `tests/test-pentax-utils.c:1084`).

Read-only init probe:

```
$ examples/pentax-safe-focus "Pentax K-1 Mark II (PTP mode)" usb:001,006 init
selected_model=Pentax K-1 Mark II (PTP mode) usb=25fb:0183 port=usb:001,006
Pentax session already open from a previous connection; observing camera state.
Pentax init stage vendor enable succeeded; function flags 0x00000003.
focus_init=command-accepted cleanup=ok retries=0
```

Bounded preview, deliberately only 2 frames to limit exposure:

```
$ examples/pentax-safe-preview "Pentax K-1 Mark II (PTP mode)" usb:001,006 2
Pentax session already open from a previous connection; observing camera state.
Pentax init stage vendor enable succeeded; function flags 0x00000003.
Pentax preview stage start-PC-LV returned 0x2001.
Pentax preview stage get-frame returned 0x2001 (17207 bytes, 22 attempts, 724 ms).
frame=1 bytes=17207 valid_jpeg=yes
Pentax preview stage start-PC-LV returned 0x2001.
Pentax preview stage get-frame returned 0x2001 (28170 bytes, 11 attempts, 340 ms).
frame=2 bytes=28170 valid_jpeg=yes
completed_frames=2 cleanup=ok
```

Exit 0. No `0x02ff`, no `0x02f9`, and the body was still on the bus immediately afterwards
(`/sys/bus/usb/devices/1-4/idVendor` present).

## What this follow-up does not establish

- **The wedge is not shown to be fixed.** Only 2 frames were attempted; the earlier failure
  surfaced at frame 3. The run was deliberately capped below that point, so it cannot rule out
  reproduction at frame 3 or later.
- **No code delta explains the difference.** `camera_pentax_capture_preview` is byte-identical
  between `origin/main` and this branch (`diff` of that function returns empty). The only
  relevant delta is `pentax_jpeg_bounds()`, which runs after a frame is received and does not
  touch the PC-LV restore path. The likeliest cause of the clean run is the power cycle clearing
  stale vendor/live-view state, not a code change.
- **`valid_jpeg=yes` is still weak integrity evidence, even with the marker walk.** The probe
  validates by checking `ff d8` at the head and `ff d9` at the tail of the returned blob. A blob
  truncated at an EXIF thumbnail's EOI also ends in `ff d9`, so that check passes for both a
  complete frame and a truncated one. Byte counts differ between runs (17093/28271 on `main`
  yesterday vs 17207/28170 here) but live-view frames vary by scene content, so sizes prove
  nothing either. Real validation needs the frames written to disk and structure-walked offline;
  `pentax-safe-preview.c` does not save them. That gap is open.
- No focus command was sent in either run. `0x9016` and `0x9017` were never issued, so the
  unresolved direction mapping in `FOCUS_LIVE_VIEW_MODEL_AUDIT.md` remains untested and no lens
  movement was observed.
