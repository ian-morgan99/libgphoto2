# Pentax Hardware Testing Runbook — Junior Agent Edition

**Audience:** any agent or human performing hardware-in-the-loop testing of the
Pentax tethering code in this fork. Follow this top-to-bottom; every step below
was executed successfully on real hardware during the 2026-08-22 → 2026-08-24
sessions. Do not improvise around a failed step — stop and report instead.

**Companion documents (read after this one):**
- Workspace handover: `/home/ian/Documents/VSCodeProjects/LibGphoto2/HANDOVER.md`
  (project governing rules, widget matrix, IT2 source anchors)
- Test history: `docs/pentax/REAL_HARDWARE_TEST_LOG.md` (chronological, mandatory
  to append to)
- Wire protocol: `docs/pentax/PENTAX_WIRE_PROTOCOL.md`

---

## 1. GOVERNING RULES (non-negotiable — violating these has wedged cameras before)

1. **Never send shutter release, focus drive, live-view start, astrotracer
   activation, delete, or reset without explicit operator approval.**
2. **One variable per hardware test.** Capture a baseline first, restore it
   after, and verify the restoration in a *fresh* session.
3. **Fail-closed always.** Unknown values stay numeric (`unknown-N`). Never
   guess wire constants. Never offer choices beyond the descriptor enum.
4. **PTP acknowledgement ≠ application.** Verify writes through live conditions
   polling (0x900f), never exit codes alone.
5. **Every test gets recorded** in `REAL_HARDWARE_TEST_LOG.md` with evidence
   files under `docs/pentax/evidence/<date>/`.
6. Default builds must NOT expose capture/preview. Research capture only via
   the compile-time option documented in §3.

---

## 2. HARDWARE IDENTITIES

| Camera | PC-P/MTP PID | MSC PID | Notes |
|---|---|---|---|
| K-3 Mark III | `25fb:0189` | — | Primary body, fw 2.20 |
| K-1 Mark II | `25fb:0183` | `25fb:0182` | fw 1.02. **0182 must NEVER be added to ptp2** (storage-only personality) |
| K-01 | `25fb:0131` | — | Fail-closed correct: 0 pentax widgets |

Port numbers change on every reconnect. **Always re-derive from sysfs**, never
reuse a remembered port:

```bash
grep -H . /sys/bus/usb/devices/*/idVendor /sys/bus/usb/devices/*/devnum \
  | paste - - | grep 25fb
```

---

## 3. BUILD PROCESS

### 3.1 Toolchain

Build tools live in the project virtualenv, not the system:

```bash
source /home/ian/Documents/VSCodeProjects/LibGphoto2/.venv/bin/activate
# provides: meson, ninja  (installed via `uv tool install`; also on PATH at ~/.local/bin)
```

System packages required once (already installed on this host):
`libgphoto2-dev gphoto2 libxml2-dev libusb-1.0-0-dev libexif-dev`.

### 3.2 Configure (one-time per build tree)

```bash
cd /home/ian/Documents/VSCodeProjects/LibGphoto2/libgphoto2
export PATH=$HOME/.local/bin:$PATH

# RESEARCH build (capture/preview probing enabled) — this is what testing uses:
meson setup _build --buildtype=debugoptimized \
  -Dcamlibs=ptp2,pentax,directory -Diolibs=disk,libusb1 \
  -Dpentax_research_capture=true
```

`-Dpentax_research_capture=true` sets `LIBGPHOTO2_ENABLE_PENTAX_RESEARCH_CAPTURE`
in config.h (see `meson_options.txt`). Without it, capture/preview research
paths compile out — default/distro-safe builds stay clean.

### 3.3 Compile

```bash
ninja -C _build
```

Expected: success with one **pre-existing** strncpy warning in ptp2. Any other
warning/error: stop, investigate, do not test on hardware with an unverified
build.

### 3.4 ltdl symlink workaround (REQUIRED after every fresh `meson setup`)

ltdl cannot find camlibs in meson subdirectories:

```bash
ln -sf _build/camlibs/ptp2/ptp2.so _build/camlibs/ptp2.so
ln -sf _build/camlibs/pentax/pentax.so _build/camlibs/pentax.so
ln -sf _build/camlibs/directory/directory.so _build/camlibs/directory.so
```

### 3.5 Verify the library under test is actually the built one

Before ANY hardware session, confirm the `.so` is newer than its sources:

```bash
ls -l --time-style=full-iso _build/camlibs/ptp2/ptp2.so camlibs/ptp2/config.c camlibs/ptp2/library.c
```

If a source file is newer, rebuild. Testing against a stale `.so` produced a
false "fix didn't work" result earlier in the project — always check mtimes.

---

## 4. RUNTIME ENVIRONMENT (export before EVERY gphoto2/harness invocation)

### 4.1 Standard CLI environment

```bash
B=/home/ian/Documents/VSCodeProjects/LibGphoto2/libgphoto2/_build
export LD_LIBRARY_PATH=$B/libgphoto2:$B/libgphoto2_port/libgphoto2_port
export CAMLIBS=$B/camlibs
export IOLIBS=$B/libgphoto2_port/libusb1:$B/libgphoto2_port/usbscsi
```

Use the **system** `gphoto2` binary (`/usr/bin/gphoto2`) — it picks up the fork
libraries purely through these variables. Do NOT install the fork over the
system copy.

### 4.2 Standalone probe harnesses (e.g. `tests/bulb_probe`) — staged IOLIBS

Meson does NOT name the port drivers where ltdl expects them for dlopen-by-name.
Stage copies first:

```bash
mkdir -p /tmp/iolibs
cp $B/libgphoto2_port/libusb1/usb1.so    /tmp/iolibs/libusb1.so
cp $B/libgphoto2_port/usbscsi/usbscsi.so /tmp/iolibs/usbscsi.so
export IOLIBS=/tmp/iolibs
# LD_LIBRARY_PATH must ALSO include the port core lib, or the iolib fails with
# undefined symbol gp_port_usb_get_sys_device:
export LD_LIBRARY_PATH=$B/libgphoto2:$B/libgphoto2_port/libgphoto2_port
```

Note: sandboxed terminals intermittently show "Read-only file system" for
`/tmp` — retry, or stage under the workspace instead.

### 4.3 Building a probe harness

```bash
/home/ian/Documents/VSCodeProjects/LibGphoto2/tests/build_bulb_probe.sh ./bulb_probe
# usage: ./bulb_probe <hold_ms> <port> <release_mode>
# release_mode=2 is the bulb-open candidate (per K-3 III traces); 0 = verified still path
```

---

## 5. VIRTUAL ENVIRONMENTS — WHAT EACH IS FOR

| Environment | Path | Purpose |
|---|---|---|
| Python venv | `/home/ian/Documents/VSCodeProjects/LibGphoto2/.venv` | Build tooling (meson, ninja). Activate before building. Also holds PE-parsing libs (dnfile, pefile, olefile, cabarchive) used for IT2 installer analysis. |
| Wine prefix | `/home/ian/Documents/VSCodeProjects/LibGphoto2/it2wine` | Runs the Windows Image Transmitter 2 v2.62E installer/app (`wine`, system package). Used ONLY for behavioural reference, never for driving the cameras under test. |
| System CLI | `/usr/bin/gphoto2` | The actual test driver, pointed at fork libs via env vars (§4.1). |
| Entangle GUI | `LibGphoto2/run-entangle.sh` | Visual tethered-shooting sanity check. Script kills GVFS holders, exports fork env, execs entangle. |

Do not pip-install anything into the system Python. Do not recreate the Wine
prefix (it took a full IT2 install; see `it2wine/mono_install.log`).

---

## 6. USB ATTACH / DETACH PROCEDURE (this is where juniors most often go wrong)

### 6.1 Before EVERY session: kill GVFS claimants

GNOME's GVFS daemons (`gvfsd-gphoto2`, `gvfs-gphoto2-volume-monitor`,
`gvfsd-mtp`) claim the camera on plug-in AND respawn automatically. `fuser`
sometimes misses them — kill by scanning `/proc/*/fd`:

```bash
for p in $(pgrep -f "gvfs|nautilus"); do
  ls -la /proc/$p/fd 2>/dev/null | grep -q "bus/usb" && kill -9 $p
done
```

Reduce recurrence (already done on this host, re-check if symptoms return):

```bash
gsettings set org.gnome.desktop.media-handling automount false
gsettings set org.gnome.desktop.media-handling automount-open false
```

Symptom if skipped: `Could not claim the USB device` /
`Could not claim interface 0 (Device or resource busy)`.

### 6.2 Derive the port fresh each time

```bash
grep -H . /sys/bus/usb/devices/*/idVendor /sys/bus/usb/devices/*/devnum \
  | paste - - | grep 25fb
```

Ports observed across sessions: K-1 II has appeared as usb:001,004 → 001,007;
K-3 III as usb:001,009. Never hardcode.

### 6.3 Camera-side USB mode: PTP/MTP vs Mass Storage (CURRENT BLOCKER)

Both cameras must be in **MTP/PTP** USB mode (camera menu: USB connection mode).
If they enumerate as USB Mass Storage (`usb-storage` bound, e.g. devices
001,011 / 001,012, PID `25fb:0182` for K-1 II), **PTP testing is impossible** —
no software fix exists; the operator must switch the camera menu back to MTP.
Check with:

```bash
lsusb | grep 25fb        # 0189/0183 = good (PTP); 0182 = MSC, ask operator
```

Evidence of the blocked state: `docs/pentax/evidence/2026-08-24/bulb_probe_*.log`
(shows only `USB Mass Storage raw SCSI` loading).

### 6.4 Detach between tests

No special teardown needed — just let the gphoto2 process exit cleanly (it
closes the vendor session; the camera resets LV/session-scoped props on close,
which is expected and documented). For physical replug: unplug, wait ~5 s,
re-run §6.1 then §6.2.

### 6.5 Wedge recovery (last resort)

A wedged PTP session shows: OpenSession → `0x02fa`, then I/O timeouts.
Escalation order:
1. Kill your own processes holding the port.
2. `usbreset <bus>,<dev>` (USB-level reset) — worked at USB level but did NOT
   recover PTP in the 2026-08-24 0x9016 wedge incident.
3. Ask the operator for a **physical power cycle** of the camera. This was the
   only thing that recovered the K-1 II after the 0x9016 wedge (~40 min of no
   self-recovery, then device vanished from the bus).

Prevention beats recovery: see the focus-drive precondition rules in
`REAL_HARDWARE_TEST_LOG.md` (never 0x9016/0x9017 without LV active + AF mode
confirmed).

---

## 7. BRANCHES AND MERGE STATE

| Branch | State | Purpose |
|---|---|---|
| `master` (= `origin/master`) | Integration branch. All tested work lands here. | Work from this branch. |
| `backup/pre-rebase-2026-08-24` | Local-only, intentionally NOT merged | Safety snapshot taken before a history rewrite on 2026-08-24. Keep; do not delete; do not merge. |
| `upstream/*` remotes | Read-only reference | `upstream` = gphoto/libgphoto2. Never push there. |

Remote feature branches (`origin/c99-compliance-pentax`, `origin/fix-pentax-deps`,
`origin/pentax-modern-integration`, etc.) are historical; `master` supersedes
them. Do not resume work on them without operator instruction.

**Rule:** hardware-test only code that is committed on `master` (or a clearly
named branch off it), and record the exact commit hash in the test-log entry.

---

## 8. TEST EXECUTION PROTOCOL (per session)

1. **Pre-flight**
   - Confirm build freshness (§3.5) and export env (§4).
   - Kill GVFS (§6.1), derive port (§6.2), confirm PTP mode (§6.3).
   - Read current state: `gphoto2 --get-config /main/status/pentaxconditions`
     (single TEXT blob: exposure mode, ISO, aperture, drive, bulb, battery…).
   - Record baseline values in the log entry.
2. **Gotcha — camera/model selection.** Always pass the specific model string,
   e.g. `--camera "Pentax K-3 Mark III (MTP mode)"`. The generic
   `USB PTP Class Camera` match leaves usb_vendor/product unset, vendor enable
   never fires, and every pentax* widget errors before I/O.
3. **One variable at a time**: set → verify via conditions poll (5×100 ms) →
   restore → verify restore in a FRESH session.
4. **Debug logging** for every command:
   ```bash
   gphoto2 --debug --debug-logfile=<name>.log ... 
   ```
   Save logs into `docs/pentax/evidence/<date>/`.
5. **Wire-level diagnostic rule** (from 2026-08-24): a SET that returns OK after
   ~120 ms with ZERO 0x400c interrupt events is silently ignored; OK after
   ~300–450 ms WITH 0x400c events means a real state change. Use IRQ presence
   to distinguish accept-vs-ignore when read-back is unavailable.
6. **Transient errors**: `PTP_RC_DeviceBusy` (0x2019) right after a write →
   wait ~3 s, retry once. Not a defect.
7. **Post-session**: verify camera responsive (`--summary`), battery level,
   baseline restored. Append entry to `REAL_HARDWARE_TEST_LOG.md`. Commit code +
   docs + evidence together.

---

## 9. CURRENT STATUS & QUEUED WORK (as of 2026-08-25)

**Proven on hardware (both bodies unless noted):** ISO/shutter/aperture/EV/WB/
drive writes with live verification; LV zoom; focus peaking (d02b); PC-LV
(d035); bracketing mode/step (d014/d015, K-3 III, LV-gated, session-scoped);
CI mode (d020); composition adjust gate (d02a — silently ignored even mid-LV,
parked); movie mode (d039, session-scoped); AF-position write (K-1 II);
old-focus drive 0x9016 (K-1 II, AF-precondition enforced); still
capture+transfer+finalize (K-1 II).

**Queued, in order:**
1. **Operator action needed:** switch both cameras from MSC back to MTP/PTP
   (§6.3). Everything below is blocked on this.
2. K-1 II bulb probe: `tests/bulb_probe` with `<hold_ms>=1500`, release_mode=2
   (operator-approved hold only). Build per §4.3, env per §4.2.
3. Duration-aware capture timeout (library.c, committed 2026-08-24) needs its
   first hardware verification — a long-bulb-timer capture that would have
   exceeded the old fixed 60 s budget.
4. Cross-process persistent-LV end-to-end verification of d02a/d02b requires a
   resident 0x9006 poller (research harness extension; see test log cont. 2–3).

---

*Runbook authored 2026-08-25 consolidating the 2026-08-22 → 2026-08-24 sessions.
Update it whenever an environment fact changes — stale instructions are worse
than none.*

## Appendix: model coverage status (post-firmware cross-check)

Status of Pentax/Ricoh bodies against the IT2 opcode set implemented here,
after cross-checking the released firmware images (k1II_v251, k3III_v220)
and the vendor download list:

| Model            | USB PID  | Status in camlib | Notes |
|------------------|----------|------------------|-------|
| K-3 III          | 0x0189   | Supported (hw-verified) | Primary test body; Astro Photo Assist opcodes reverse-engineered from k3III_v220. |
| K-3 III Mono     | 0x018f   | Supported (research)    | Shares model_no 78420 via prefix match. |
| K-1 II           | 0x0183   | Supported (hw-verified) | Bulb capture + transfer verified; zero-byte-download issue tracked separately. |
| KP               | 0x017f   | Supported (unverified on hw) | Same IT2 family; needs a bench pass. |
| K-70             | 0x017d   | Supported (unverified on hw) | As above. |
| K-1              | 0x0179   | Supported (unverified on hw) | Prefix "PENTAX K-1" — must not shadow K-1 II. |
| K-3              | 0x0165   | Supported (unverified on hw) | Prefix "PENTAX K-3". |
| GR III           | 0x210f   | Supported (unverified on hw) | Fixed-lens; capture path differs. |
| 645D             | 0x0130   | Supported (unverified on hw) | ext_version 0 legacy path. |
| **645Z**         | 0x0167   | Supported (firmware-derived) | PID extracted from fwdc224b.bin v1.30 header (`fb25 6601 6701`); second position = PTP mode per KP precedent (0x017f). Needs bench pass. |
| KF               | 0x018e   | Supported (firmware-derived) | Same method, fwdc245b.bin v1.33 (`fb25 8d01 8e01`). Needs bench pass. |
| GR IIIx          | unverified | Do not add yet        | Same — likely prefix variant of GR III entry once confirmed. |
| K-3 II           | —        | Deliberately excluded   | No IT2 reference implementation; older protocol generation. |

Rule: never add a model ID without either (a) a hardware plug-in session
logging its PID/`DeviceModel`, or (b) an extracted firmware image that
contains the string/PID pair.
