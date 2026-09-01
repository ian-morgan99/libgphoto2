# STAGE 2.1: Vendor-Mode Handshake (Prerequisite)
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