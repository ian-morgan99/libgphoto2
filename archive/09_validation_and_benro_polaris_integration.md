# VALIDATION AND BENRO POLARIS INTEGRATION

> **OBSOLETE:** Historical draft only. Use B1–B3 in [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md).

## Stage 3.1: Hardware Validation
**Entrance Criteria**: Stage 2 complete.
**Activities**:
- Test all implemented features against real Pentax K-3 III and/or K-1 II hardware.
- Test property get/set, live view, file download, capture control, focus control, status feedback.
- Verify model-specific gating works.
- Document any discrepancies.
**Exit Criteria**:
- Core functionality working: live view, file download, capture control.
- Model-specific gating working.
- Status blob parsing providing useful feedback.
- Limitations documented.

## Stage 3.2: Benro Polaris Firmware Integration
**Entrance Criteria**: Stage 3.1 complete.
**Activities**:
- Build custom libgphoto2 with Pentax support for ARM target (using Benro Polaris patcher's build environment).
- Integrate with Benro Polaris firmware patcher.
- Build test firmware image.
- Flash test image (if appropriate and hardware available) or test via USB host mode.
- Test end-to-end functionality via Polaris interface.
**Exit Criteria**:
- Custom libgphoto2 built successfully for ARM target.
- Integrated with Benro Polaris firmware patcher.
- End-to-end functionality tested via Polaris interface (if possible).
- Any issues documented.

## Stage 3.3: Alternative Validation (If Hardware Flashing Not Feasible)
**Entrance Criteria**: Stage 3.1 complete.
**Activities**:
- Validate via USB connection with standard gphoto2 CLI first.
- Then test the same build via Benro Polaris in USB host mode (if supported).
- Or validate through simulation/testing without flashing.
**Exit Criteria**:
- Core functionality validated via available methods.
- Implementation readiness for Polaris integration documented.
