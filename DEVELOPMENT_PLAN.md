# Development Plan: Adding Pentax Support to libgphoto2 Fork

This document specifies exactly what changes to make to the user's fork of libgphoto2 (ian-morgan99/libgphoto2) to implement full Pentax camera support based on analysis of Pentax Image Transmitter 2. The plan is broken into incremental, testable stages.

## Prerequisites
- Working libgphoto2 build environment (meson/autotools)
- Access to Pentax K-3 III and/or K-1 II hardware for validation
- Base fork is up-to-date with upstream (as of current date)
- Local copy of Image Transmitter 2 source available for reference

## Overview
We will implement support in two parts:
1. **Part 1: Basic Generic PTP Support** - Provides parity with upstream Monochrome K-3 III (PR #1273): detection, configuration, file download.
2. **Part 2: Pentax-Specific Extensions** - Adds live view, advanced capture, extensive configuration, model-specific gating, and status feedback.

Each part is divided into stages with clear entrance/exit criteria and specific code changes.

---

# PART 1: BASIC GENERIC PTP SUPPORT

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

---

# PART 2: PENTAX-SPECIFIC EXTENSIONS

## Goal
Add Pentax-specific capabilities: live view, advanced capture control, extensive configuration, model-specific gating, and status feedback.

### Stage 2.1: Vendor-Mode Handshake (Prerequisite)
**Goal**: Implement `SetPentaxVendorMode` (0x9001) as required before any vendor-specific ops.

**Files to Modify**:
- `ptp2/ptp.c` – Add handling for opcode 0x9001.
- `ptp2/config.c` – No changes needed (handshake is an operation, not a property).
- `ptp2/mtp.h` – Add opcode definition if not present.

**Exact Changes**:
In `ptp2/ptp.c`, in the switch statement handling PTP opcodes (look for the large switch), add:
```c
case 0x9001: /* SetPentaxVendorMode */
{
    uint32_t model_no, vendor_mode, vendor_ext_version, dummy5;
    uint16_t res_code;
    uint32_t func_avail_flag;

    if (ptp_get_data(params, ptp, 5, &model_no, &vendor_mode, &vendor_ext_version, &dummy5, &dummy5) < 0)
        return PTP_RC_InvalidParameter;

    /* Call vendor-specific function (to be implemented) */
    res_code = pentax_set_vendor_mode(params, model_no, vendor_mode, vendor_ext_version, &func_avail_flag);
    if (res_code != PTP_RC_OK)
        return res_code;

    /* Return function availability flag as uint32 data */
    ptp_emit_uint32(params, func_avail_flag);
    return PTP_RC_OK;
}
```
Also implement the helper function `pentax_set_vendor_mode` (to be defined later, likely in a new file or in `ptp2/ptp.c`).

**Exit Criteria**:
- Opcode 0x9001 recognized and calls handler.
- Handler returns PTP_RC_OK on success with correct data.
- Without this handshake, vendor properties/ops should return appropriate error (e.g., PTP_RC_InvalidParameter or similar).

### Stage 2.2: Pentax-Specific Property Support
**Goal**: Add Pentax vendor property codes (0xD0xx range) to `ptp2/config.c`.

**Files to Modify**:
- `ptp2/config.c` – Add entries for each Pentax vendor property in the appropriate tables (getters/setters).
- `ptp2/mtp.h` – Add property code definitions if missing.

**Exact Changes**:
In `ptp2/config.c`, find the arrays defining properties (look for `static const struct _CamPropDesc camprops[]` or similar). Add entries for each property code from the analysis. Example for ShutterSpeed (0xD00F):
```c
{ 0xD00F, "shutterspeed", GP_CAPTURE_SHUTTER_SPEED, PT_PTP_TYPE_UINT32, 0, 0, 0, 0 },
```
But we must map each to the appropriate libgphoto2 property (GP_CAPTURE_*). For Pentax-specific ones without direct GP_CAPTURE_* equivalent, we may need to add new GP_CAPTURE_* constants or use generic integer properties. However, libgphoto2 already has many properties; we should map to existing ones where possible.

Given the extensive list, we will add all properties from the analysis. For brevity in this plan, we specify the approach:

- For each property code in the 0xD0xx range from the analysis, add an entry in the property table in `ptp2/config.c` with:
  - The property code.
  - A descriptive name (e.g., "colortemp" for 0xD018).
  - The corresponding libgphoto2 property constant (e.g., GP_CAPTURE_WHITE_BALANCE_TEMPERATURE if exists, else we may need to define a new one in `libgphoto2/gphoto2-port.h`).
  - The appropriate PT_PTP_TYPE_* (UINT32, UINT16, BYTE, etc.) based on the analysis (most are UINT32 or BYTE arrays).
  - Get/min/max/step as appropriate (many are enumerations; we may need to fetch the range via GetDevicePropDesc or use fixed ranges if known).

Given the complexity, we will implement a subset first and expand. However, the plan is to add all.

**Exit Criteria**:
- All Pentax vendor property codes from the analysis have entries in `ptp2/config.c`.
- Properties are gettable and settable where appropriate (some may be read-only).
- Values returned are within expected ranges/enums.

### Stage 2.3: Core Pentax Opcodes Implementation
**Goal**: Implement the high-confidence opcodes with clear call sites.

**Files to Modify**:
- `ptp2/ptp.c` – Add cases for each opcode in the main switch.
- `ptp2/mtp.h` – Add opcode definitions if missing.
- Possibly `ptp2/ptp.h` for helper function declarations.
- New helper functions may be added to `ptp2/ptp.c` or a new file.

**Exact Changes**:
We will add cases for each opcode. Below are the exact implementations for each.

**Note**: All these operations should first check if vendor mode is enabled (via a global or per-port flag). We will add a helper `pentax_vendor_mode_enabled(PTPParams *params)` that checks a flag set by the handshake.

**Example skeleton for a vendor operation**:
```c
if (!pentax_vendor_mode_enabled(params))
    return PTP_RC_InvalidParameter; // or specific error

/* proceed with operation */
```

**Implement each**:

1. **0x9006 GetLiveViewFrameData** (DATA-TO-READ)
```c
case 0x9006: /* GetLiveViewFrameData */
{
    if (!pentax_vendor_mode_enabled(params))
        return PTP_RC_InvalidParameter;
    /* Call helper to get live view frame data */
    uint8_t *frame_data;
    uint32_t frame_size;
    uint16_t res_code = pentax_get_live_view_frame(params, &frame_data, &frame_size);
    if (res_code != PTP_RC_OK)
        return res_code;
    ptp_emit_data(params, frame_data, frame_size);
    /* Assuming we allocated frame_data, free it */
    free(frame_data);
    return PTP_RC_OK;
}
```

2. **0x9007 GetPentaxSubImage** (DATA-TO-READ)
```c
case 0x9007: /* GetPentaxSubImage */
{
    if (!pentax_vendor_mode_enabled(params))
        return PTP_RC_InvalidParameter;
    uint32_t object_handle = 0; /* or get from params if needed */
    uint8_t *img_data;
    uint32_t img_size;
    uint16_t res_code = pentax_get_subimage(params, object_handle, &img_data, &img_size);
    if (res_code != PTP_RC_OK)
        return res_code;
    ptp_emit_data(params, img_data, img_size);
    free(img_data);
    return PTP_RC_OK;
}
```

3. **0x9008 GetPentaxMainImage** (DATA-TO-READ) – similar to above.

4. **0x900B GetTransferCandidateFileInfo** (DATA-TO-READ)
```c
case 0x900B: /* GetTransferCandidateFileInfo */
{
    if (!pentax_vendor_mode_enabled(params))
        return PTP_RC_InvalidParameter;
    uint8_t type_of_trans_img; /* byte */
    if (ptp_get_data(params, ptp, 1, &type_of_trans_img) < 0)
        return PTP_RC_InvalidParameter;
    /* Call helper */
    uint8_t *resp_buf;
    uint32_t resp_size;
    uint16_t res_code = pentax_get_transfer_candidate_file_info(params, type_of_trans_img, &resp_buf, &resp_size);
    if (res_code != PTP_RC_OK)
        return res_code;
    ptp_emit_data(params, resp_buf, resp_size);
    free(resp_buf);
    return PTP_RC_OK;
}
```
The response format per analysis: [0]=FileFormat, [1]=Quality, [2]=ImageSize, [3]=nameLen*2, then UTF-16 name.

5. **0x900C GetCamFileOperationCommand** (DATA-TO-READ)
```c
case 0x900C: /* GetCamFileOperationCommand */
{
    if (!pentax_vendor_mode_enabled(params))
        return PTP_RC_InvalidParameter;
    /* Call helper */
    uint8_t *resp_buf;
    uint32_t resp_size;
    uint16_t res_code = pentax_get_cam_file_operation_command(params, &resp_buf, &resp_size);
    if (res_code != PTP_RC_OK)
        return res_code;
    ptp_emit_data(params, resp_buf, resp_size);
    free(resp_buf);
    return PTP_RC_OK;
}
```
Response: byte[0]=FileOperation, bytes[1-4]=int32 OperationInfo.

6. **0x900D GetTranferFileDataBlock** (DATA-TO-READ)
```c
case 0x900D: /* GetTranferFileDataBlock */
{
    if (!pentax_vendor_mode_enabled(params))
        return PTP_RC_InvalidParameter;
    uint32_t transRequestSize;
    if (ptp_get_data(params, ptp, 1, &transRequestSize) < 0)
        return PTP_RC_InvalidParameter;
    uint8_t *readData;
    uint32_t transferredDataSize;
    uint16_t res_code = pentax_get_transfer_file_block(params, transRequestSize, &readData, &transferredDataSize);
    if (res_code != PTP_RC_OK)
        return res_code;
    ptp_emit_data(params, readData, transferredDataSize);
    free(readData);
    return PTP_RC_OK;
}
```

7. **0x900F GetAllConditions** (DATA-TO-READ)
```c
case 0x900F: /* GetAllConditions */
{
    if (!pentax_vendor_mode_enabled(params))
        return PTP_RC_InvalidParameter;
    uint8_t *resp_buf;
    uint32_t resp_size;
    uint16_t res_code = pentax_get_all_conditions(params, &resp_buf, &resp_size);
    if (res_code != PTP_RC_OK)
        return res_code;
    ptp_emit_data(params, resp_buf, resp_size);
    free(resp_buf);
    return PTP_RC_OK;
}
```
We will need to parse this blob in the host side (see status blob parsing below). For now, just emit the raw data.

8. **0x9011 InitiatePentaxCapture** (NO-DATA)
```c
case 0x9011: /* InitiatePentaxCapture */
{
    if (!pentax_vendor_mode_enabled(params))
        return PTP_RC_InvalidParameter;
    uint32_t releaseMode, focusMode, mwbMode, syncMode, apertureReset;
    if (ptp_get_data(params, ptp, 5, &releaseMode, &focusMode, &mwbMode, &syncMode, &apertureReset) < 0)
        return PTP_RC_InvalidParameter;
    uint16_t res_code = pentax_initiate_capture(params, releaseMode, focusMode, mwbMode, syncMode, apertureReset);
    if (res_code != PTP_RC_OK)
        return res_code;
    return PTP_RC_OK;
}
```

9. **0x9012 TerminatePentaxCapture** (NO-DATA)
```c
case 0x9012: /* TerminatePentaxCapture */
{
    if (!pentax_vendor_mode_enabled(params))
        return PTP_RC_InvalidParameter;
    uint32_t releaseMode;
    if (ptp_get_data(params, ptp, 1, &releaseMode) < 0)
        return PTP_RC_InvalidParameter;
    uint16_t res_code = pentax_terminate_capture(params, releaseMode);
    if (res_code != PTP_RC_OK)
        return res_code;
    return PTP_RC_OK;
}
```

10. **0x9013 InterruptFunction** (NO-DATA)
```c
case 0x9013: /* InterruptFunction */
{
    if (!pentax_vendor_mode_enabled(params))
        return PTP_RC_InvalidParameter;
    uint16_t res_code = pentax_interrupt_function(params);
    if (res_code != PTP_RC_OK)
        return res_code;
    return PTP_RC_OK;
}
```

11. **0x9016 FocusControl** (NO-DATA)
```c
case 0x9016: /* FocusControl */
{
    if (!pentax_vendor_mode_enabled(params))
        return PTP_RC_InvalidParameter;
    uint32_t driveAmount, driveDirection;
    if (ptp_get_data(params, ptp, 2, &driveAmount, &driveDirection) < 0)
        return PTP_RC_InvalidParameter;
    uint16_t res_code = pentax_focus_control(params, driveAmount, driveDirection);
    if (res_code != PTP_RC_OK)
        return res_code;
    return PTP_RC_OK;
}
```

12. **0x9017 FocusControlNew** (NO-DATA)
```c
case 0x9017: /* FocusControlNew */
{
    if (!pentax_vendor_mode_enabled(params))
        return PTP_RC_InvalidParameter;
    uint32_t imagePlaneDisplacement;
    if (ptp_get_data(params, ptp, 1, &imagePlaneDisplacement) < 0)
        return PTP_RC_InvalidParameter;
    uint16_t res_code = pentax_focus_control_new(params, imagePlaneDisplacement);
    if (res_code != PTP_RC_OK)
        return res_code;
    return PTP_RC_OK;
}
```

13. **0x9018 SetCompositionAdjustmentOffset** (NO-DATA)
```c
case 0x9018: /* SetCompositionAdjustmentOffset */
{
    if (!pentax_vendor_mode_enabled(params))
        return PTP_RC_InvalidParameter;
    uint32_t adjustDirection, step;
    if (ptp_get_data(params, ptp, 2, &adjustDirection, &step) < 0)
        return PTP_RC_InvalidParameter;
    uint16_t res_code = pentax_set_composition_adjustment_offset(params, adjustDirection, step);
    if (res_code != PTP_RC_OK)
        return res_code;
    return PTP_RC_OK;
}
```

14. **0x9019 SetFlagToKeepAperturePosition** (NO-DATA)
```c
case 0x9019: /* SetFlagToKeepAperturePosition */
{
    if (!pentax_vendor_mode_enabled(params))
        return PTP_RC_InvalidParameter;
    uint32_t keepFlag, apertureValue;
    if (ptp_get_data(params, ptp, 2, &keepFlag, &apertureValue) < 0)
        return PTP_RC_InvalidParameter;
    uint16_t res_code = pentax_set_flag_to_keep_aperture_position(params, keepFlag, apertureValue);
    if (res_code != PTP_RC_OK)
        return res_code;
    return PTP_RC_OK;
}
```

**Helper Functions**: We need to implement the `pentax_*` helper functions. These will likely be placed in `ptp2/ptp.c` or a new file `ptp2/pentax.c`. For simplicity, we can add them to `ptp2/ptp.c` as static functions.

**Vendor Mode Flag**: We need a way to track whether vendor mode is enabled per camera session. We can add a flag to the `PTPParams` or `PTPCamera` structure. Since modifying core structures may be invasive, we can use a property or a side channel. However, the simplest is to add a boolean to `PTPParams` (if we can modify `ptp.h`). Given that we are already modifying `ptp2/`, we can add a field to `PTPParams` in `ptp2/ptp.h` if it's safe.

Alternatively, we can store the state in the camera's private data (if using the camera struct). Since this is getting complex, we may opt for a simpler approach: require that vendor mode is set via a property or we check a global flag set by the handshake. For a first implementation, we can use a static/global variable per port (not thread-safe but acceptable for single camera). Better to add a flag to `PTPParams`.

Given the scope, we will outline the helper functions without full implementation details, as the exact implementation depends on how we store the vendor mode state.

**Exit Criteria for Stage 2.3**:
- All high-confidence opcodes (0x9001, 0x9006-0x900D, 0x900F, 0x9011-0x9013, 0x9016-0x9019) are implemented in `ptp2/ptp.c`.
- Each opcode checks vendor mode enabled (via a helper).
- Each opcode calls a helper function that performs the operation.
- Helper functions are stubbed to return appropriate values (we will implement them in later stages).
- The code compiles without errors.

### Stage 2.4: Model-Specific Capability Gating
**Goal**: Implement `_camModelNo` approach to gate features based on model capabilities.

**Files to Modify**:
- `ptp2/ptp.c` – Add model detection and capability checking.
- `ptp2/mtp.h` – Add model number constants and capability flag definitions if needed.
- Possibly `ptp2/config.c` – to gate property get/set based on capabilities.

**Exact Changes**:
We will add a function `pentax_get_model_capabilities(PTPParams *params)` that:
1. Reads the camera's `Model` string via standard PTP `GetDeviceInfo` (property 0xD001? Actually GetDeviceInfo is opcode 0x0101, but we can use the existing PTP device info retrieval).
2. Maps the model string to a `_camModelNo` and capability flags (exactly as in `MtpDevice.cs` Model setter).
3. Returns a structure with flags.

Then, in each property get/set and opcode handler, we check if the requested feature is supported for the current model before proceeding.

**Implementation Steps**:
- Add a struct `PentaxCameraCapabilities` with boolean fields for each capability (dual slot, new transfer, etc.).
- Add a function `pentax_get_model_capabilities` that returns this struct.
- In property get/set, before accessing the property, check if the property is supported for the model (e.g., if trying to set movie mode on a non-movie model, return error).
- In opcode handlers, similarly check if the opcode is supported for the model.

**Exact Code Sketch**:
In `ptp2/ptp.c`, add:
```c
typedef struct {
    int dual_slot;
    int new_transfer_mode;
    int new_focus_fine_control;
    int movie_supported;
    int movie_settings_supported;
    int movie_sub_image_supported;
    int composition_adj_supported;
    int aperture_keep_supported;
    int color_temp_setting_supported;
    int ci_detail_param_supported;
    int self_timer_continuous_new_supported;
    int remote_continuous_new_supported;
    int slot_change_supported;
    int pc_lv_high_res_supported;
    /* add more as needed */
} PentaxCameraCapabilities;

static PentaxCameraCapabilities pentax_get_model_capabilities(PTPParams *params)
{
    PentaxCameraCapabilities caps = {0};
    /* Get Model string via standard PTP GetDeviceInfo (we can reuse existing code) */
    char model[256];
    int ret = ptp_get_device_prop_value(params, PTP_DPC_Model, model, sizeof(model));
    if (ret < 0) {
        /* unable to get model, assume no capabilities */
        return caps;
    }
    /* Map model to capabilities exactly as in MtpDevice.cs Model setter */
    if (strncmp(model, "PENTAX K-3 Mark III", 19) == 0) {
        caps.dual_slot = 1;
        caps.new_transfer_mode = 1;
        caps.new_focus_fine_control = 1;
        caps.movie_supported = 1;
        caps.movie_settings_supported = 1;
        caps.movie_sub_image_supported = 1;
        caps.composition_adj_supported = 1;
        caps.aperture_keep_supported = 1;
        caps.color_temp_setting_supported = 1;
        caps.ci_detail_param_supported = 1;
        caps.self_timer_continuous_new_supported = 1;
        caps.remote_continuous_new_supported = 1;
        caps.slot_change_supported = 1;
        caps.pc_lv_high_res_supported = 1;
    } else if (strncmp(model, "PENTAX K-3", 10) == 0) {
        /* K-3 (no marks) */
        caps.dual_slot = 1;
        /* all others 0 */
    } else if (strncmp(model, "PENTAX 645Z", 11) == 0) {
        caps.dual_slot = 1;
        /* etc. */
    }
    /* ... and so on for each model ... */
    return caps;
}
```

Then, in each property getter/setter and opcode handler, we do:
```c
PentaxCameraCapabilities caps = pentax_get_model_capabilities(params);
if (!caps.some_feature) {
    return PTP_RC_InvalidParameter; // or specific error
}
```

**Exit Criteria for Stage 2.4**:
- Function `pentax_get_model_capabilities` implemented.
- Model detection via `GetDeviceInfo` works.
- Capability flags match the analysis exactly for each model.
- Property get/set and opcode handlers check capabilities and return appropriate errors for unsupported features on a given model.
- Works correctly for at least two different Pentax models if available.

### Stage 2.5: Status Blob Parsing (`GetAllConditions` 0x900F)
**Goal**: Parse the `GetAllConditions` response to provide status feedback.

**Files to Modify**:
- `ptp2/ptp.c` – In the handler for 0x900F, instead of just emitting raw data, we will parse it and emit meaningful status information via libgphoto2's status reporting mechanism (or we can store the parsed data in the camera's private data and provide getters for status properties).
- We may need to add new status properties to libgphoto2 (e.g., battery level, storage state, etc.) or use existing ones.

**Approach**:
Rather than emitting raw data, we will parse the blob and update the camera's private data (if we have a camera struct) or we can emit the parsed data as separate properties via the standard PTP property system? However, the status blob is not a standard PTP property; it's a vendor-specific opcode.

Simpler: In the handler for 0x900F, we parse the blob and store the relevant status fields in the camera's private data (if we extend the camera struct). Then we can provide getters for these status fields as standard PTP properties (e.g., we could map battery level to the standard battery level property if it makes sense, or we create new vendor-specific status properties).

Given the complexity, we can start by simply emitting the raw data and letting the host application parse it (if the host application is modified). But for a generic libgphoto2 improvement, we want to make the status available via standard properties.

We will add new properties to `ptp2/config.c` for the status fields we want to expose, and in their getters, we will compute the value from the last cached `GetAllConditions` blob.

**Exact Changes**:
1. Add a struct to hold the last `GetAllConditions` blob (or parsed fields) in the camera's private data (if we have one) or in a global per-port variable.
2. In the handler for 0x900F, store the blob (or parse and store fields).
3. Add getters for status properties (e.g., battery level, storage state, etc.) that read from the cached data.
4. Add these properties to `ptp2/config.c` with appropriate property codes (we can use standard PTP property codes where applicable, or define new vendor-specific status property codes if needed).

Given the time, we will outline the approach:

- In `ptp2/ptp.c`, add a static struct `LastGetAllConditions` per port (or per camera) to hold the last blob or parsed values.
- In the handler for 0x900F, parse the blob and fill this struct.
- Add property getters for:
  - Battery level (if available in blob)
  - Storage state (sd1CardState, sd2CardState)
  - Remaining space (sd1Remain, sd2Remain)
  - Capture mode info
  - Mode enable info
  - Astrotracer phase/state/time limit
  - Pixel shift status (read-only indicator)
  - Current writing slot
  - etc.

We will map these to existing libgphoto2 properties where possible (e.g., battery level maps to GP_CAPTURE_BATTERY_LEVEL, storage state may not have a direct equivalent, we may need to use generic integer properties).

**Exit Criteria for Stage 2.5**:
- `GetAllConditions` (0x900F) handler implemented.
- Last blob parsed and stored.
- At least 80% of the documented status fields are available as gettable properties (standard or vendor-specific).
- Values update correctly when the blob changes.
- Pixel shift status correctly reported as read-only indicator.
- Astrotracer phase/state correctly reported.

### Stage 2.6: Model-Specific Property Gating (Optional Refinement)
**Goal**: Ensure property get/set handlers check model capabilities before allowing access.

**Files to Modify**:
- `ptp2/config.c` – In property getters/setters, call `pentax_get_model_capabilities` and check if the property is supported for the model.

**Exact Changes**:
For each property getter/setter in `ptp2/config.c`, add:
```c
PentaxCameraCapabilities caps = pentax_get_model_capabilities(params);
if (!caps.some_feature_related_to_this_property) {
    return PTP_RC_InvalidParameter; // or specific error
}
```
before accessing the property.

**Exit Criteria**:
- Property get/set handlers correctly return errors for unsupported properties on a given model.
- Works correctly for at least two models.

### Stage 2.7: Integration and Polish
**Goal**: Ensure code follows libgphoto2 standards, no memory leaks, passes existing tests.

**Files to Modify**:
- All modified files.
- Possibly `ptp2/ptp.h` for new function declarations.

**Activities**:
- Run `make check` or equivalent test suite.
- Check for memory leaks (if tools available).
- Ensure code style matches surrounding code.
- Document any limitations in the source comments.

**Exit Criteria**:
- Code compiles without warnings.
- Existing libgphoto2 test suite passes (no regressions).
- Code follows libgphoto2 coding style.
- Limitations documented in source comments.

**Stage Gate 2 Complete**: When all Pentax-specific features are implemented, tested, and integrated without regressions.

---

# VALIDATION AND BENRO POLARIS INTEGRATION

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

---

# SUMMARY OF FILES TO MODIFY

Below is a consolidated list of files that will be modified, with a brief description.

### `ptp2/ptp.c`
- Add vendor-mode handshake handler (0x9001).
- Add handlers for all high-confidence opcodes (0x9006-0x900D, 0x900F, 0x9011-0x9013, 0x9016-0x9019).
- Add helper functions for each operation (stubs to be filled).
- Add vendor mode enabled check (requires storing state).
- Add model detection and capability checking functions.
- Add status blob parsing and caching (for 0x900F).
- Possibly add fields to `PTPParams` or camera struct for vendor mode flag and cached status.

### `ptp2/config.c`
- Add entries for all Pentax vendor property codes (0xD0xx range) from the analysis.
- In property getters/setters, add model capability checks (optional refinement).
- Add getters for status properties derived from cached `GetAllConditions` blob (if implementing status feedback via properties).

### `ptp2/mtp.h`
- Add definitions for any missing opcode codes (0x9xxx range).
- Add definitions for any missing property codes (if not already present).

### `ptp2/ptp.h`
- Add declarations for new helper functions (pentax_* functions, pentax_get_model_capabilities, etc.).
- Possibly add a field to `PTPParams` for vendor mode flag (if we modify the struct).

### Optional: New file `ptp2/pentax.c`
- Could house all the helper functions to keep `ptp2/ptp.c` clean.

### Optional: `libgphoto2/gphoto2-port.h`
- If we need to define new GP_CAPTURE_* constants for properties that don't have existing equivalents, we would add them here. However, we should first try to map to existing properties.

---

# EXIT CRITERIA FOR THE ENTIRE PROJECT

The project will be considered complete when:
1. Pentax K-3 III and/or K-1 II cameras are detectable and identifiable.
2. Core camera properties (ISO, shutter, aperture, WB, focus) are gettable and settable.
3. Live view frame acquisition works.
4. File download and transfer works.
5. Capture triggering and control works.
6. Model-specific capability gating works correctly.
7. Status blob parsing provides useful feedback (at least battery, storage, capture mode, astrotracer state, pixel shift indicator).
8. No regressions in existing libgphoto2 functionality.
9. Implementation follows libgphoto2 coding standards.
10. Limitations and known issues are clearly documented (e.g., pixel shift read-only only, HDR LS-series only, etc.).

This development plan provides a detailed, step-by-step roadmap with specific code changes that can be evaluated for certainty by other agents. Each stage has clear entrance and exit criteria, allowing for incremental development and testing.