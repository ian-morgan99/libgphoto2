# Process Document: Pentax Support Integration into libgphoto2

This document outlines the process followed to analyze Pentax Image Transmitter 2 and plan libgphoto2 integration.

## Phase 1: Initial Reconnaissance
- Located user's libgphoto2 fork: https://github.com/ian-morgan99/libgphoto2
- Identified the problematic `pentaxmodern` camlib attempt
- Confirmed upstream already has basic USB ID support for K-01, K-1 II, K-3 III in legacy SCSI driver
- Found upstream PR #1273 adding working PTP support for K-3 Mark III Monochrome (validated)

## Phase 2: Source Acquisition
- Located Image Transmitter 2 decompiled C# source on SMB share:
  `\\morganbackup.local\home\Projects\Pentax\ImageTransmitter2\IMAGETransmitter2\`
- Verified access via gvfs mount at `/run/user/1000/gvfs/smb-share:server=morganbackup.local,share=home/Projects/Pentax/ImageTransmitter2/IMAGETransmitter2/`
- Copied source to local workspace for reliable access:
  `/home/ian/Documents/VSCodeProjects/LibGphoto2/ImageTransmitter2/IMAGETransmitter2/`

## Phase 3: Opcode and Property Extraction
- Extracted complete opcode table from `MtpOpCode.cs`
- Mapped all `MtpSetDevicePropValue` and `MtpGetDevicePropValue` calls to property codes
- Identified the vendor-mode handshake prerequisite (0x9001 SetPentaxVendorMode)
- Confirmed live view, file download, and capture trigger opcodes exist

## Phase 4: Capability Matrix Construction
- Analyzed `MtpDevice.cs` Model property setter to build per-model capability flags
- Extracted all value table enums (LUT files) for exposure modes, drive modes, etc.
- Analyzed `ConditionIndex.cs` for named indices into status blobs
- Mapped `GetAllConditions` (0x900F) status blob offsets from usage in `MtpDevice.cs`

## Phase 5: Feature Analysis
- Determined Pixel Shift Resolution is read-only status (offset 320, bit 1024) - no setter
- Confirmed HDR mode is LS-series-only program mode value, not general Pentax control
- Verified ISO/shutter speed are dynamic PTP GetDevicePropDesc queries (no static tables needed)
- Mapped all controllable properties via `MtpSetDevicePropValue`

## Phase 6: Integration Strategy Formulation
- Decided against reviving `pentaxmodern` as separate camlib (too many bugs, architectural issues)
- Recommended extending existing generic PTP driver (`ptp2/`) with Pentax vendor extensions
- Outlined three-phase implementation: properties → status blob → opcodes
- Specified model-specific handling using `_camModelNo` approach mirroring Image Transmitter 2

## Phase 7: Documentation
- Generated `CameraCapabilities.md` with complete capability matrix
- Created this `PROCESS.md` to document methodology
- Saved working notes in `/memories/repo/pentax-libgphoto2-project.md`

## Validation Requirements
Before implementation, validate against real hardware:
1. Confirm PTP-mode USB IDs for K-01, K-1 II, K-3 III
2. Test property get/set for core settings (ISO, shutter, aperture, WB, focus, drive mode)
3. Verify live view frame acquisition via 0x9006
4. Verify file download via 0x900D (GetTranferFileDataBlock)
5. Verify capture trigger via 0x9011 (InitiatePentaxCapture)
6. Verify status blob parsing for key indicators (battery, ISO, WB, astrotracer phase, pixel shift flag)
7. Test model-specific capability gating

## Risk Assessment
- **Low Risk**: Property get/set, live view, file download, basic capture trigger
- **Medium Risk**: Status blob parsing (requires careful offset validation)
- **Low Risk**: Model-specific gating (straightforward mapping)
- **Unknown Risk**: Unexplored opcodes (InvokeCameraTask, GeneralCommand sub-commands) - treat as low priority until evidence emerges

## Dependencies
- Requires working libgphoto2 build environment (meson/autotools)
- Requires access to Pentax K-3 III and/or K-1 II hardware for validation
- Benefits from user's existing Benro Polaris patcher pipeline for end-to-end firmware testing
