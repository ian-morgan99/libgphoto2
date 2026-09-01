# STAGE 2.1: Vendor-Mode Handshake (Prerequisite)

> **OBSOLETE:** Despite its filename, this is not the canonical implementation plan. Use P1–P3 in [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md).
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
    uint32_t model_no, vendor_mode, vendor_ext_version;
    uint16_t res_code;
    uint32_t func_avail_flag;

    /* Construct PTPContainer for vendor mode command */
    PTPContainer ptp;
    PTP_CNT_INIT(ptp, PTP_OC_PENTAX_SetVendorMode);
    
    /* Set the 5 parameters as specified in the analysis:
       13 (vendor extension ID), model_no, vendor_mode, vendor_ext_version, 0 */
    ptp.params.Nparam = 5;
    ptp.params.param[0] = 13; /* Pentax vendor extension ID */
    ptp.params.param[1] = model_no;
    ptp.params.param[2] = vendor_mode;
    ptp.params.param[3] = vendor_ext_version;
    ptp.params.param[4] = 0; /* dummy5 */
    
    /* Send command with no data phase */
    res_code = ptp_transaction(params, &ptp, PTP_DP_NODATA, 0, NULL, NULL);
    if (res_code != PTP_RC_OK)
        return res_code;

    /* Extract function availability flag from response parameter 1 */
    /* Based on analysis of ExecuteCommandWithoutDataPhase in MtpDevice.cs */
    func_avail_flag = ptp.params.param[1];
    
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
- Correctly sends the 5 parameters: 13, model_no, vendor_mode, vendor_ext_version, 0
- Correctly extracts function availability flag from response parameter 1
