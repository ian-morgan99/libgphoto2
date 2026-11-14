# Pentax camera-window operative cards

Status: prepared offline on 2026-11-14. This document is an execution card, not
hardware evidence. It is deliberately model-specific and fail-closed because
the available live-view battery window is short.

## Governing boundary

Run the camera in two separate lanes:

1. **Lane A — direct PC/libgphoto2.** Establishes whether the exact libgphoto2
   checkout performs the operation at its own boundary. Use this lane first.
2. **Lane C — real Polaris.** Establishes packaging, firmware, transport, and
   consumer integration only after Lane A passes. Do not use a Polaris result to
   justify changing libgphoto2.

The evidence rule is:

```text
direct PC fail + Polaris fail at the same first PTP operation
    candidate libgphoto2 defect; reproduce and trace before changing code
direct PC pass + Polaris fail
    packaging/runtime/firmware/consumer defect; do not change libgphoto2
direct PC pass + Polaris pass + OpenPolaris fail
    OpenPolaris/client defect
```

A `0x2001` acknowledgement, a zero return code, a shutter sound, or a file
alone is not a successful setting or capture result. A successful result needs
the source-defined read-back/application check, restoration, and fresh-session
verification where applicable.

## Evidence locks before the camera is powered for testing

The current direct-PC candidate is:

```text
libgphoto2 repository: /home/ian/Documents/VSCodeProjects/LibGphoto2/libgphoto2
required ref: main
qualified source SHA: 1bf8ba51a83144f16872fa8cdca6d386bc9d191f
```

Do not substitute a branch, detached checkout, dirty tree, or a newly built
library without recording its exact SHA. Re-run the host gate if the SHA or
build changes:

```sh
cd /home/ian/Documents/VSCodeProjects/LibGphoto2/libgphoto2
git status --short --branch
git rev-parse HEAD
bash tests/run-pentax-host-check.sh
```

The prepared host gate passed 26/26 non-hardware tests for the SHA above. The
run produced an isolated build and installed `ptp2.so` with SHA-256
`0708c6b5b4b407f00f577b7e57ec3ecdf7aae58a2a7346990659be813148ae27`; retain
the newly printed artifact directory rather than assuming that temporary path
still exists.

The relevant built direct-PC cards are:

```text
examples/pentax-safe-preview
examples/pentax-safe-iso-roundtrip
examples/pentax-safe-shutter-roundtrip
examples/pentax-safe-focus
examples/pentax-safe-lvaf-roundtrip
examples/pentax-persistent-config-benchmark
```

The Bulb qualification probe is research-only and must be built and run from
its source contract in `tests/pentax-bulb-qualification-probe.c`; it is not a
general capture test.

Before any hardware command, preserve in the evidence directory:

```text
camera model and exact body firmware reported by the camera
USB VID:PID and fresh usb:BUS,DEVICE identity
libgphoto2 SHA and installed ptp2.so SHA-256
build/install paths and loader variables
camera mode, live-view state, AF state, drive mode, image format, NR/Pixel Shift
battery state and operator initials
```

The later IMAGE Transmitter 2 reference copy remains the capability authority:
`MtpDevice.cs` hash
`e8d11ed772aad3731d7eabd0c06fb0174aae859c5a1f6d241bf73b6d046de059`.
Use the source anchors and model gates in `PENTAX_WIRE_PROTOCOL.md`,
`REFERENCE_CLIENTS.md`, `FOCUS_LIVE_VIEW_MODEL_AUDIT.md`, and
`IMAGE_TRANSMITTER_SETTING_PATH.md`. Do not infer a Pentax operation from an
opcode name, DeviceInfo, a descriptor, an adjacent model, or another client.

Polaris firmware v104/v105/v106 labels are not sufficient provenance for a
camera conclusion. The Polaris lane must carry the exact firmware package/hash,
installed `/app/FwVer`, packaged libgphoto2 provenance, and Patcher SHA. If any
one is missing, classify the result as unknown integration evidence.

## Battery-minimised execution order

Do the following in one PC session, stopping at the first failed gate. Keep
live view off until the preview gate and never start a long preview run merely
to collect a number.

### Card A0 — attach and identify, no camera writes

The camera is connected to the PC, not Polaris. Kill only known local desktop
claimants, derive the USB port fresh, and stop if the body enumerates as MSC:

```sh
grep -H . /sys/bus/usb/devices/*/idVendor /sys/bus/usb/devices/*/devnum \\
  | paste - - | grep 25fb
```

Use only the PTP/MTP identity for the selected model:

```text
K-1 Mark II: 25fb:0183, `Pentax K-1 Mark II (PTP mode)`
K-3 Mark III: 25fb:0189, `Pentax K-3 Mark III (MTP mode)`
25fb:0182 is K-1 Mark II MSC and must not enter the ptp2 path.
```

Record the exact body firmware from the camera menu or the source-faithful
summary path. Do not assume the previous K-1 II firmware 1.02 observation is
the current body.

Use the isolated build from the fresh host gate, not the system camlib:

```sh
BUILD=/path/to/the/fresh/isolated/host-gate/build
export LD_LIBRARY_PATH="$BUILD/libgphoto2:$BUILD/libgphoto2_port/libgphoto2_port"
export CAMLIBS="$BUILD/camlibs/ptp2"
export IOLIBS="$BUILD/libgphoto2_port/libusb1"
gphoto2 --version
```

The operator must verify from debug output that the intended `ptp2.so` is
loaded. A successful `gphoto2 --version` without loader evidence is not a
qualification.

### Card A1 — source-faithful init and read-only baseline

Run exactly one init card for the attached model and save all output:

```sh
"$BUILD/examples/pentax-safe-focus" \
  "Pentax K-1 Mark II (PTP mode)" "usb:BUS,DEVICE" init
```

For a K-3 III, substitute the exact K-3 III model string. Do not send focus,
capture, live-view, or cleanup retries if init fails. Then collect the
source-owned conditions/config baseline with a debug log:

```sh
gphoto2 --debug --debug-logfile="$EVIDENCE/conditions-baseline.log" \
  --camera "MODEL STRING" --port "usb:BUS,DEVICE" \
  --get-config /main/status/pentaxconditions
```

Record the advertised values and raw conditions. The IT2 path obtains live
exposure values from periodic `GetAllConditions`; a descriptor `CurrentValue`
alone is not the post-write oracle.

### Card A2 — bounded live-view integrity

The current source includes marker-walk JPEG EOI detection. Validate the
smallest useful sample first:

```sh
mkdir -p "$EVIDENCE/preview-1"
"$BUILD/examples/pentax-safe-preview" \
  "Pentax K-1 Mark II (PTP mode)" "usb:BUS,DEVICE" 1 \
  "$EVIDENCE/preview-1"
```

Use the K-3 III model string for a K-3 III. Only if the one-frame run has a
complete SOI/EOI JPEG, a valid marker walk, successful original-state restore,
and the body remains present, run five frames:

```sh
mkdir -p "$EVIDENCE/preview-5"
"$BUILD/examples/pentax-safe-preview" \
  "MODEL STRING" "usb:BUS,DEVICE" 5 "$EVIDENCE/preview-5"
```

Stop immediately on a `0x02ff`/`0x02f9` response, zero-length frame, malformed
JPEG, restore failure, or USB disappearance. Do not proceed to focus or capture
after a wedge. The 2026-11-09 K-1 II preview run wedged after frames 1 and 2;
that is a recovery/input to this bounded card, not proof that a later source
SHA has the same result.

Do not run 50 or 500 frames until a new one-frame and five-frame run pass on the
exact current SHA. Those longer counts are endurance follow-ups, not the first
camera-window test.

### Card A3 — one-variable settings round trip

Run settings only after the preview gate passes. Each program chooses and
checks a source-advertised value, restores the original, and requires a fresh
session check. Save the debug trace and the program output.

K-1 II ISO:

```sh
"$BUILD/examples/pentax-safe-iso-roundtrip" \
  "Pentax K-1 Mark II (PTP mode)" "usb:BUS,DEVICE"
```

Shutter card (K-1 II or K-3 III, with that exact model string):

```sh
"$BUILD/examples/pentax-safe-shutter-roundtrip" \
  "MODEL STRING" "usb:BUS,DEVICE"
```

Do not add a guessed target or sweep values. A setting passes only when the
changed live condition is read back, the original value is restored exactly,
and a fresh session independently sees the original value.

### Card A4 — focus, only after live view is good

The body must be in the physical AF/live-view state required by IT2, and the
operator must be watching the lens. Use one bounded source-defined command:

```sh
"$BUILD/examples/pentax-safe-focus" \
  "Pentax K-1 Mark II (PTP mode)" "usb:BUS,DEVICE" near-lv
```

For the K-3 III use its exact model string. Run `far-lv` only after the near
operation is observed and the lens is safe. Do not use the old no-live-view
9016 path as a retry after a wedge. K-1 II and K-3 III use different IT2 focus
operations; cross-model opcode or magnitude reuse is prohibited.

### Card A5 — Bulb last, explicit operator gate

Bulb is a capture test, not a readiness test. Do it only with an operator
present, adequate battery, a verified baseline, and a disposable/known output
directory. The K-3 III research probe is limited to one or two interactive
captures:

```sh
probe --execute "Pentax K-3 Mark III (MTP mode)" \
  usb:BUS,DEVICE "$EVIDENCE/bulb" 1
```

Use the exact built probe path and source usage from
`tests/pentax-bulb-qualification-probe.c`; do not replace `probe` with an
unverified binary. The probe requires a TTY and explicit operator interaction.
Do not infer K-1 II Bulb support from K-3 III evidence or from a Polaris log.

For every capture, require the complete lifecycle: source-defined release,
completion/stop, output publication, readable file, and appropriate EXIF or
metadata. If the body accepts a command but output publication is pending or
the camera state is uncertain, stop and preserve the evidence; do not send a
later shutter command as a recovery attempt.

### Card A6 — close and fresh-session check

After any changed setting or capture, restore the original state, end the
session cleanly, reconnect only if needed, and run the read-only baseline once
more. Record battery and USB presence. A clean process exit is not a physical
recovery claim.

## Polaris handoff card C0

Move the camera cable only after Lane A has passed the intended operation and
all PC processes have exited. Confirm the PC no longer owns the camera before
moving the cable. On Polaris, first collect package/runtime provenance and
logs; do not begin with Bulb.

Use the existing Patcher qualification plan, which has the correct tested
ordering and explicit Bulb gate:

```sh
cd /home/ian/Documents/VSCodeProjects/BenroPolarisPatcher
python3 scripts/camera-qualification-round.py --plan
python3 scripts/camera-qualification-round.py \
  --host 192.168.0.1 --outdir /tmp/polaris-camera-qualification
```

The first real Polaris run must preserve `/app/FwVer`, the packaged
`/app/openpolaris-libgphoto2-provenance.txt`, Patcher SHA, package manifest,
and pre/post Mlog/Clog snapshots. Verify the camera's fresh USB identity in
the Polaris logs. A cached camera name or a previously observed PID is not
enough.

Run the Polaris Bulb phase only last and only with physical presence:

```sh
python3 scripts/camera-qualification-round.py \
  --host 192.168.0.1 --outdir /tmp/polaris-camera-bulb \
  --with-bulb --i-am-present
```

Keep the default 45-second exposure cap. Treat `BLOCKED`, `UNKNOWN`, and
`CODE_COMPLETE_UNVERIFIED` as non-passes. A `DEVICE_VERIFIED` result must still
name the exact first failing/passing lower-level operation and preserve the
logs and output artifact.

## Evidence packet and decision record

Use one immutable evidence directory per camera session, for example
`/tmp/pentax-window-YYYYMMDD-HHMMSS-model`. Store raw debug logs, conditions
before/after, preview bytes or hashes, output files, commands, and the exact
source/package manifests. Never overwrite an earlier session.

At the end write a short result with these fields:

```text
lane: A-PC or C-Polaris
camera/model/body firmware:
first operation under test:
libgphoto2 SHA / ptp2 SHA-256:
Patcher SHA / Polaris FwVer / packaged provenance: (C only)
first failing PTP operation and response:
application/read-back evidence:
restoration and fresh-session evidence:
physical observation:
classification: PASS / FAIL / BLOCKED / UNKNOWN
next permitted action:
```

Only a repeated direct-PC failure at the same source-defined operation is a
candidate for a libgphoto2 change. Any such change must first add the smallest
deterministic regression coverage and repeat the generic PTP2 impact review.
