# K-3 III Regression Test Suite

**Purpose:** Prevent regression of working features on the Pentax K-3 III. Run after every libgphoto2 change that touches ptp2/Pentax code paths.

**Test environment:** Benro Polaris gimbal + Pentax K-3 III, patched firmware (o-v9k or later), OpenPolaris app.

---

## 1. JPEG Capture (Rock Solid)

**Expected:** Single JPEG captured, transferred to host, no error. Repeatable indefinitely.

```bash
# CLI test
gp-camera capture --target=memory --type=image | head -c 200 | xxd | head -3
# Should show JPEG magic bytes: ff d8 ff e0

# Repeat 5 times rapidly
for i in 1 2 3 4 5; do
  gp-camera capture --target=memory --type=image > /dev/null && echo "Shot $i OK" || echo "Shot $i FAIL"
done
```

**Pass criteria:** All 5 shots succeed, no "camera busy", no timeout.

---

## 2. RAW Capture (DNG)

**Expected:** Single DNG captured and transferred.

```bash
# Set to RAW mode first
gp-camera set-config imagequality=raw
gp-camera capture --target=memory --type=image | head -c 4 | xxd
# Should show DNG magic: 49 49 42 00 (II*\0)
```

**Pass criteria:** DNG file received, valid header.

---

## 3. RAW+JPEG Dual-Format (Issue #73 - Fixed)

**Expected:** Both RAW and JPEG members captured. Second shot must NOT fail with "camera busy".

```bash
gp-camera set-config imagequality=raw+jpeg

# Shot 1
gp-camera capture --target=memory --type=image > /dev/null && echo "Shot 1 OK" || echo "Shot 1 FAIL"

# Shot 2 - THIS IS THE REGRESSION TEST (previously failed with CAMERA_BUSY)
gp-camera capture --target=memory --type=image > /dev/null && echo "Shot 2 OK" || echo "Shot 2 FAIL"

# Shot 3 - verify sustained operation
gp-camera capture --target=memory --type=image > /dev/null && echo "Shot 3 OK" || echo "Shot 3 FAIL"
```

**Pass criteria:** All 3 shots succeed. No "stale transfer candidate" error.

---

## 4. Autofocus (Center Point, M Mode)

**Expected:** AF engages, lens moves to focus, returns success.

```bash
# Set to AF-S (single-shot AF)
gp-camera set-config focusmode=afs

# Trigger AF
gp-camera capture --target=memory --type=image
# Observe: lens should move and lock focus
```

**Pass criteria:** Lens physically moves to focus, no timeout, image captured.

---

## 5. Manual Focus Direction (Issue - Fixed in af1f94997)

**Expected:** Positive MF drive = lens moves toward Near (closer). Negative = Far (farther).

```bash
# Set to MF mode
gp-camera set-config focusmode=mf

# Drive near (positive)
gp-camera set-config manualfocusdrive=1
# Observe: lens should move toward closer focus

# Drive far (negative)
gp-camera set-config manualfocusdrive=-1
# Observe: lens should move toward farther focus
```

**Pass criteria:** Direction matches sign. Near = positive, Far = negative.

---

## 6. Hot-Swap Camera via USB Cable Pull

**Expected:** Pulling USB cable and reconnecting (or swapping camera) works without app crash.

**Manual test:**
1. Connect K-3 III, verify preview working
2. Pull USB cable from camera
3. Wait 5 seconds
4. Reconnect USB cable
5. Verify connection re-established, preview resumes

**Pass criteria:** No app crash, connection recovers within ~30 seconds.

---

## 7. Lens Swap While Connected

**Expected:** Changing lens while connected to Polaris works.

**Manual test:**
1. Connect camera with lens A
2. Pull USB cable
3. Remove lens A, attach lens B
4. Reconnect USB cable
5. Verify connection and preview

**Pass criteria:** Camera recognized with new lens, no crash.

---

## 8. Battery Change Handling

**Expected:** Camera handles battery removal/insertion gracefully.

**Manual test:**
1. Connect camera, verify working
2. Remove battery (camera powers off)
3. Insert fresh battery
4. Power on camera
5. Verify connection recovers

**Pass criteria:** No permanent "camera busy" state after battery change.

---

## 9. Performance / Responsiveness

**Expected:** Preview and capture responsive, no excessive lag.

**Manual test:**
1. Start preview
2. Observe frame rate (should be smooth, not stuttering)
3. Trigger capture, measure time to completion

**Pass criteria:** Preview smooth, capture completes in < 5 seconds for JPEG.

---

## 10. Mode Dial Change While Connected

**Expected:** Changing camera mode dial while connected doesn't crash the app.

**Manual test:**
1. Connect camera in M mode
2. Change mode dial to A (aperture priority)
3. Verify connection still alive
4. Trigger AF or capture

**Pass criteria:** No app crash, connection maintained.

---

## Known Remaining Issues (Not Regression Tests)

| Issue | Status | Notes |
|-------|--------|-------|
| Version displays 8.0.0.76 instead of build ID | Open | FwVer override not reaching displayed `sw:` field |
| MF buttons need variable speeds | Open | All MF drives currently same speed |
| App crash on AF after mode change | Open | Requires investigation |
| Camera connection not released on app crash | Open | Requires USB compat toggle or restart |

---

## K-1 II Issues (Separate from K-3 III)

The following are K-1 II specific and should be tracked separately:

- Very laggy preview
- Parameters screen crashes
- EV and WB settings don't stick
- MF doesn't work
- AF doesn't work
- Shutter release takes continuous shots instead of single

---

**Last updated:** 2026-09-14
**Author:** Ian Morgan
