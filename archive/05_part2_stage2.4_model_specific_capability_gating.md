# STAGE 2.4: Model-Specific Capability Gating

> **OBSOLETE:** Historical draft only. Use P2–P3 in [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md).
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
