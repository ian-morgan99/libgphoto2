# Phase 2 Expansion Strategy: KP, 645Z, and Other Pentax Models

Revision: 2026-08-25

## Overview

This document outlines the strategy for expanding LibGphoto2 Pentax support beyond the K-1 II and K-3 III to include other models such as the **KP**, **645Z**, and legacy bodies. Phase 1 (K-1 II & K-3 III stabilization) must be completed first before initiating Phase 2 work in a separate fork or feature branch.

---

## Phase 1 Completion Criteria (K-1 II & K-3 III)

Before starting Phase 2, the following Phase 1 items must be complete:

### 1. d02a Composition Adjust Half-Press/AF Flow
- **Current status**: SET `01` gets OK (~7 ms) but read-back stays 00 with NO 0x400c event — silent-ignore signature even during active LV.
- **Required investigation**: Actual composition-adjust capture flow (half-press/AF sequence), not just the flag write.
- **Vendor opcode**: `0x9018` (composition direction/steps; two response values)

### 2. Cross-process persistent-LV end-to-end verification of d02a/d02b
- **Required**: A resident 0x9006 poller to verify focus peaking (d02b) works while LV is active, and composition adjust (d02a) behavior during live view.

### 3. K-1 II / K-3 III hardware test coverage complete
- All vendor opcodes (`0x9001` through `0x9022`) mapped and tested
- All direct properties (`0x5001`, `0x5005`, `0x5007`, etc.) verified
- Duration-aware capture timeout fully validated

---

## Phase 2 Model Discovery Strategy

### Step 1: Model Capability Matrix Extension

Extend the `IMAGE_TRANSMITTER_CAPABILITY_MATRIX.md` to include:

| Gate | K-1 Mark II | K-3 Mark III | KP | 645Z |
|---|---|---|---|---|
| Exact model / USB | PC-P `25fb:0183`; MSC `25fb:0182` | `25fb:0189`; MSC identity target | [to verify] | [to verify] |
| Vendor model / extension | 78400 / 1 | 78420 / 1 | [to verify] | [to verify] |
| New focus control (`0x9017`) | No | Yes | [to verify] | [to verify] |
| Old focus control (`0x9016`) | Yes | No | [to verify] | [to verify] |
| Composition adjust (`0x9018`) | No | Yes | **Yes** (IT2 confirms) | [to verify] |
| Aperture keep (`0x9019`) | No | Yes | [to verify] | [to verify] |
| Bracketing mode/step (`d014/d015`) | Withheld | Yes | [to verify] | **Yes** (IT2 confirms) |
| Movie settings/sub-image | No | Yes | [to verify] | Yes (likely) |
| Current-writing-slot / high-res PC LV | No | Yes | [to verify] | [to verify] |

### Step 2: IT2 Source Code Verification

From `IT2_2625_decompile/RemoteAssistant/MtpDevice.cs`:

**Composition Adjust Support (`0xd02a` / `0x9018`)**:
- Line 71, 537, 561, 632, 745: `_isCompositionAdjSupported` flag
- Lines 96, 173: IT2 confirms composition adjust is supported for **K-3 III family and KP**

**Focus Control**:
- `0x9016`: old focus (amount, direction) — K-1 II only
- `0x9017`: new focus (signed displacement) — K-3 III only

**Exposure Bracketing (`0xd014/0xd015`)**:
- From `pentax-utils.c:291-293`: `_isExpBracketSupport` is set only for **K-3 III family and 645Z**
- The fork has no 645Z entry yet, so K-3 III family only until one is added

---

## Phase 2 Hardware Testing Procedure (When Hardware Available)

For each new model (KP, 645Z, etc.):

### 1. PTP/MTP Mode Verification
```bash
cd /home/ian/Documents/VSCodeProjects/LibGphoto2
B=/home/ian/Documents/VSCodeProjects/LibGphoto2/libgphoto2/_build
export LD_LIBRARY_PATH=$B/libgphoto2:$B/libgphoto2_port/libgphoto2_port:$LD_LIBRARY_PATH
export CAMLIBS=$B/camlibs
export IOLIBS=$B/libgphoto2_port/libusb1:$B/libgphoto2_port/usbscsi

/usr/bin/gphoto2 --port <usb:xxx,yyy> --summary
```

### 2. Config Tree Mapping
```bash
/usr/bin/gphoto2 --port usb:xxx,yyy --list-config > k-model-config.txt
```

Check for presence/absence of:
- `/main/status/pentaxconditions` (K-3 III / K-1 II have this; K-01 does not)
- `/main/capturesettings/shutterspeed`, `iso`, `aperture`
- `/main/status/bulb-timer`, `/main/status/bulb-seconds`

### 3. Vendor Opcode Testing
Test vendor operations:
- `0x9001`: vendor mode; no data; `[13,model,on/off,1,0]`; Param2 flags
- `0x9006`: live-view frame, data-in
- `0x900f`: all conditions, data-in, no parameters (568-byte HW-R)
- `0x9016`: old focus (K-1 II)
- `0x9017`: new focus (K-3 III)
- `0x9018`: composition direction/steps (K-3 III, KP)
- `0x9019`: aperture keep flag/value (K-3 III)

### 4. Direct Property Testing
Test PTP properties:
- `0x5001`: battery level
- `0x5005`: white balance
- `0x5007`: aperture
- `0x5010`: exposure compensation
- `0xd00f`: shutter/Bulb timer
- `0xd013`: drive mode
- `0xd014/d015`: bracket mode/step (K-3 III, 645Z)
- `0xd018`: WB color temperature (K-3 III UI gate)

---

## K-01 Findings Summary (Legacy Model Pattern)

The **K-01** represents a legacy PTP pattern that differs from K-1 II / K-3 III:

### What K-01 HAS:
- Basic PTP properties via `/main/other/*` paths:
  - `/main/other/5001`: Battery Level (UINT8, 0-100)
  - `/main/other/5011`: Date & Time (string)
  - `/main/other/d406`: Session Initiator Info (string)
  - `/main/other/d407`: Perceived Device Type (UINT32, value: 1)
  - `/main/other/d303`: Unknown property (UINT8, value: 1)
- Standard MTP object properties for storage enumeration
- Vendor Extension ID: `0x6` (microsoft.com/DeviceServices: 1.0)

### What K-01 LACKS:
- **NO** `/main/status/pentaxconditions` TEXT blob
- **NO** `/main/capturesettings/shutterspeed`, `iso`, `aperture` config paths
- **NO** live view capture via vendor opcodes (no 0x9006 support confirmed)
- **NO** focus peaking (d02b) or composition adjust (d02a)

### Implications for Phase 2:
Legacy models like K-01 may require a separate PTP property mapping path, similar to how MSC (USB Mass Storage) is handled for the K-1 II's `25fb:0182` identity. The `/main/other/*` pattern suggests these bodies use standard PTP/MTP properties rather than Pentax-specific vendor conditions.

---

## Recommended Phase 2 Branch Strategy

### Option A: Separate Feature Branch in Main Fork
```bash
cd /home/ian/Documents/VSCodeProjects/LibGphoto2/libgphoto2
git checkout -b feature/phase2-model-expansion
```

**Pros**: Keeps all Pentax work in one repository; easier to share capability matrix updates.

**Cons**: May clutter the main branch history before Phase 1 is fully complete.

### Option B: Separate Fork for Phase 2
Create a new fork like `LibGphoto2-pentax-phase2` or `LibGphoto2-pentax-legacy-models`.

**Pros**: Clean separation; Phase 1 stabilization remains uncluttered; easier to manage different firmware requirements.

**Cons**: Requires maintaining two forks/branches; potential for duplicated effort if both branches evolve independently.

### Recommendation: **Option B (Separate Fork)**
Once K-1 II and K-3 III support is rock solid, create a separate fork or feature branch specifically for Phase 2 model expansion. This ensures:
1. Phase 1 stabilization remains clean and focused
2. Different firmware thresholds and PTP patterns can be isolated
3. Hardware testing for KP/645Z/legacy models doesn't interfere with K-1 II/K-3 III validation

---

## Next Steps (Post-Phase 1)

When Phase 1 is complete and hardware for KP/645Z becomes available:

1. **Create Phase 2 fork or feature branch**
2. **Run PTP/MTP mode verification** for each new model via `gphoto2 --summary`
3. **Map config tree structure** to determine if `/main/status/pentaxconditions` exists or if legacy `/main/other/*` pattern applies
4. **Verify vendor opcodes** (`0x9001`, `0x9006`, `0x900f`, `0x9016/0x9017`, `0x9018`, `0x9019`)
5. **Test direct properties** (battery, white balance, aperture, exposure compensation, shutter timer, drive mode, bracketing)
6. **Update `IMAGE_TRANSMITTER_CAPABILITY_MATRIX.md`** with new model entries
7. **Append test results to `REAL_HARDWARE_TEST_LOG.md`**

---

## References

- `libgphoto2/docs/pentax/IMAGE_TRANSMITTER_CAPABILITY_MATRIX.md` — Complete IT2 UI-to-wire setting audit and capability matrix
- `libgphoto2/docs/pentax/HARDWARE_TESTING_RUNBOOK.md` — Governing runbook for hardware testing; §1 governing rules, §8.5 SET verification via conditions poll, §9 queued work
- `IT2_2625_decompile/RemoteAssistant/MtpDevice.cs` — IT2 source confirming composition adjust support for K-3 III family and KP, bracketing support for K-3 III family and 645Z
- `libgphoto2/camlibs/ptp2/pentax-utils.c` — Fork implementation of `_isCompositionAdjSupported`, `_isExpBracketSupport`, focus control gates
