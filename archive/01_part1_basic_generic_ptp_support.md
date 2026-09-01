# PART 1: BASIC GENERIC PTP SUPPORT

> **OBSOLETE:** Historical draft only. Use the canonical [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md).

## Goal
Enable detection, configuration (ISO, shutter, aperture, WB, focus), and file download for Pentax models that respond to standard PTP (expected: K-3 III, K-1 II, K-01, etc.). This does NOT require vendor mode.

### Stage 1.1: Environment Setup and Baseline
**Entrance Criteria**: Prerequisites met.
**Activities**:
- Ensure libgphoto2 builds from source.
- Verify Pentax hardware is detected by `gphoto2 --auto-detect`.
- Confirm basic summary obtainable.
**Exit Criteria**:
- libgphoto2 builds without errors.
- Pentax hardware detected.
- `gphoto2 --summary` returns basic info.
**No code changes needed yet** (just verification).

### Stage 1.2: Standard PTP Property Support
**Goal**: Make standard PTP properties (ISO, shutter, aperture, WB, focus) gettable and settable where supported.

**Files to Modify**:
- `ptp2/config.c` – Ensure standard PTP properties are handled (they likely already are; verify no regression).
- No new code needed if standard PTP already works; we just need to test and confirm.

**Activities**:
- Test `gphoto2 --get-config iso`, `--get-config shutterspeed`, etc.
- Test `gphoto2 --set-config iso=200`, etc.
- Map any missing properties if needed (unlikely).

**Exit Criteria**:
- At least ISO, aperture, shutter speed, white balance, focus mode are gettable.
- At least ISO and aperture are settable (others may be read-only on some models).
- Error handling for unsupported properties works.

### Stage 1.3: File Transfer Support
**Goal**: Enable file listing and download.

**Files to Modify**:
- `ptp2/ptp.c` – Ensure standard PTP object handling works (likely already functional).
- No new code needed if standard PTP object operations are intact.

**Activities**:
- Test `gphoto2 --list-files`.
- Test `gphoto2 --get-file all`.
- Verify file integrity and speed.

**Exit Criteria**:
- File listing works.
- File download works for at least one format (JPEG/RAW).
- Download speed reasonable.
- Error handling for unsupported operations works.

### Stage 1.4: Basic Event Handling (Optional)
**Goal**: Add basic event handling if desired (not required for core function).

**Files to Modify**:
- `ptp2/ptp.c` – Ensure event handling is enabled and functional.

**Activities**:
- Test `--wait-event` or similar.
- Verify no interference with synchronous ops.

**Exit Criteria**:
- Event system functional without breaking synchronous operations.

**Stage Gate 1 Complete**: When Pentax hardware is detectable, core properties gettable/settable, file listing/download works, and no regressions.
