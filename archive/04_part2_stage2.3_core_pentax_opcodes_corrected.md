# STAGE 2.3: Core Pentax Opcodes Implementation (CORRECTED)

> **OBSOLETE AND TECHNICALLY INCORRECT:** This still uses nonexistent responder-side APIs. Use P1–P5 in [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md).
**Goal**: Implement the high-confidence opcodes with clear call sites using correct PTP transaction pattern.

**Files to Modify**:
- `ptp2/ptp.c` – Add cases for each opcode in the main switch.
- `ptp2/mtp.h` – Add opcode definitions if missing.
- Possibly `ptp2/ptp.h` for helper function declarations.
- New helper functions may be added to `ptp2/ptp.c` or a new file.

**Exact Changes**:
We will add cases for each opcode. Below are the exact implementations for each using the correct host-initiated PTP transaction pattern.

**Note**: All these operations should first check if vendor mode is enabled (via a global or per-port flag). We will add a helper `pentax_vendor_mode_enabled(PTPParams *params)` that checks a flag set by the handshake.

**Example skeleton for a vendor operation**:
```c
if (!pentax_vendor_mode_enabled(params))
    return PTP_RC_InvalidParameter; // or specific error

/* proceed with operation */
```

**Implement each with correct PTP transaction pattern**:

1. **0x9006 GetLiveViewFrameData** (DATA-TO-READ)
```c
case 0x9006: /* GetLiveViewFrameData */
{
    if (!pentax_vendor_mode_enabled(params))
        return PTP_RC_InvalidParameter;
    /* Call helper to get live view frame data */
    uint8_t *frame_data;
    uint32_t frame_size;
    uint16_t res_code = ptp_pentax_get_live_view_frame(params, &frame_data, &frame_size);
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
    uint16_t res_code = ptp_pentax_get_subimage(params, object_handle, &img_data, &img_size);
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
    uint16_t res_code = ptp_pentax_get_transfer_candidate_file_info(params, type_of_trans_img, &resp_buf, &resp_size);
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
    uint16_t res_code = ptp_pentax_get_cam_file_operation_command(params, &resp_buf, &resp_size);
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
    uint16_t res_code = ptp_pentax_get_transfer_file_block(params, transRequestSize, &readData, &transferredDataSize);
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
    uint16_t res_code = ptp_pentax_get_all_conditions(params, &resp_buf, &resp_size);
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
    uint16_t res_code = ptp_pentax_initiate_capture(params, releaseMode, focusMode, mwbMode, syncMode, apertureReset);
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
    uint16_t res_code = ptp_pentax_terminate_capture(params, releaseMode);
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
    uint16_t res_code = ptp_pentax_interrupt_function(params);
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
    uint16_t res_code = ptp_pentax_focus_control(params, driveAmount, driveDirection);
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
    uint16_t res_code = ptp_pentax_focus_control_new(params, imagePlaneDisplacement);
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
    uint16_t res_code = ptp_pentax_set_composition_adjustment_offset(params, adjustDirection, step);
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
    uint16_t res_code = ptp_pentax_set_flag_to_keep_aperture_position(params, keepFlag, apertureValue);
    if (res_code != PTP_RC_OK)
        return res_code;
    return PTP_RC_OK;
}
```

**Helper Functions**: We need to implement the `pentax_*` helper functions using the correct PTP transaction pattern. These will likely be placed in `ptp2/ptp.c` or a new file `ptp2/pentax.c`. For simplicity, we can add them to `ptp2/ptp.c` as static functions.

**Correct Implementation Pattern for Helper Functions**:
```c
/* 0x9006 GetLiveViewFrameData */
uint16_t
ptp_pentax_get_live_view_frame(PTPParams *params,
                               unsigned char **data,
                               unsigned int *size)
{
    PTPContainer ptp;
    PTP_CNT_INIT(ptp, PTP_OC_PENTAX_GetLiveViewFrameData);
    return ptp_transaction(params, &ptp, PTP_DP_GETDATA, 0, data, size);
}

/* 0x9002 GetAllConditions */
uint16_t
ptp_pentax_get_all_conditions(PTPParams *params,
                              unsigned char **data,
                              unsigned int *size)
{
    PTPContainer ptp;
    PTP_CNT_INIT(ptp, PTP_OC_PENTAX_GetAllConditions);
    return ptp_transaction(params, &ptp, PTP_DP_GETDATA, 0, data, size);
}

/* 0x9008 InitiateCapture */
uint16_t
ptp_pentax_initiate_capture(PTPParams *params)
{
    PTPContainer ptp;
    PTP_CNT_INIT(ptp, PTP_OC_PENTAX_InitiateCapture);
    return ptp_transaction(params, &ptp, PTP_DP_NODATA, 0, NULL, NULL);
}

/* 0x900C GetTransferMetadata */
uint16_t
ptp_pentax_get_transfer_metadata(PTPParams *params,
                                 uint32_t handle,
                                 unsigned char **data,
                                 unsigned int *size)
{
    PTPContainer ptp;
    PTP_CNT_INIT(ptp, PTP_OC_PENTAX_GetTransferMetadata);
    ptp.params.Nparam = 1;
    ptp.params.param[0] = handle;
    return ptp_transaction(params, &ptp, PTP_DP_GETDATA, 0, data, size);
}

/* 0x900D GetTransferDataBlock */
uint16_t
ptp_pentax_get_transfer_data_block(PTPParams *params,
                                   uint32_t handle,
                                   uint32_t offset,
                                   uint32_t size,
                                   unsigned char **data,
                                   unsigned int *datasize)
{
    PTPContainer ptp;
    PTP_CNT_INIT(ptp, PTP_OC_PENTAX_GetTransferDataBlock);
    ptp.params.Nparam = 3;
    ptp.params.param[0] = handle;
    ptp.params.param[1] = offset;
    ptp.params.param[2] = size;
    return ptp_transaction(params, &ptp, PTP_DP_GETDATA, 0, data, datasize);
}

/* 0x9003 ReceivedPentaxCreatedObject */
uint16_t
ptp_pentax_received_pentax_created_object(PTPParams *params,
                                          uint32_t handle)
{
    PTPContainer ptp;
    PTP_CNT_INIT(ptp, PTP_OC_PENTAX_ReceivedPentaxCreatedObject);
    ptp.params.Nparam = 1;
    ptp.params.param[0] = handle;
    return ptp_transaction(params, &ptp, PTP_DP_NODATA, 0, NULL, NULL);
}

/* 0x9004 CameraShutdown */
uint16_t
ptp_pentax_camera_shutdown(PTPParams *params)
{
    PTPContainer ptp;
    PTP_CNT_INIT(ptp, PTP_OC_PENTAX_CameraShutdown);
    return ptp_transaction(params, &ptp, PTP_DP_NODATA, 0, NULL, NULL);
}
```

**Vendor Mode Flag**: We need a way to track whether vendor mode is enabled per camera session. We can add a flag to the `PTPParams` or `PTPCamera` structure. Since modifying core structures may be invasive, we can use a property or a side channel. However, the simplest is to add a boolean to `PTPParams` (if we can modify `ptp.h`). Given that we are already modifying `ptp2/`, we can add a field to `PTPParams` in `ptp2/ptp.h` if it's safe.

Alternatively, we can store the state in the camera's private data (if using the camera struct). Since this is getting complex, we may opt for a simpler approach: require that vendor mode is set via a property or we check a global flag set by the handshake. For a first implementation, we can use a static/global variable per port (not thread-safe but acceptable for single camera). Better to add a flag to `PTPParams`.

Given the scope, we will outline the helper functions without full implementation details, as the exact implementation depends on how we store the vendor mode state.

**Exit Criteria for Stage 2.3**:
- All high-confidence opcodes (0x9001, 0x9006-0x900D, 0x900F, 0x9011-0x9013, 0x9016-0x9019) are implemented in `ptp2/ptp.c`.
- Each opcode checks vendor mode enabled (via a helper).
- Each opcode calls a helper function that performs the operation using the correct PTP transaction pattern (host-initiated).
- Helper functions are implemented with correct PTP transaction pattern.
- The code compiles without errors.
