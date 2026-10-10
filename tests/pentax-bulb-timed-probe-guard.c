#include "pentax-bulb-timed-probe-guard.h"

PentaxBulbProbeGuardResult
pentax_bulb_probe_validate_camera (uint16_t usb_vendor, uint16_t usb_product,
	uint32_t model_no, int supported_model, int vendor_mode_enabled)
{
	if ((usb_vendor != 0x25fbU) || (usb_product != 0x0189U) ||
	    (model_no != PENTAX_MODEL_K3_MARK_III))
		return PENTAX_BULB_PROBE_WRONG_CAMERA;
	if (!supported_model || !vendor_mode_enabled)
		return PENTAX_BULB_PROBE_NOT_READY;
	return PENTAX_BULB_PROBE_SAFE;
}

PentaxBulbProbeGuardResult
pentax_bulb_probe_validate_conditions (const PentaxConditions *conditions)
{
	if (!conditions)
		return PENTAX_BULB_PROBE_NOT_READY;
	if (conditions->activity_flags != 0 ||
	    (conditions->operation_state == 49) ||
	    (conditions->operation_state == 50))
		return PENTAX_BULB_PROBE_BUSY;
	if (conditions->exposure_mode != PENTAX_EXP_MODE_BULB)
		return PENTAX_BULB_PROBE_WRONG_MODE;
	if (!(conditions->capability_flags & PENTAX_CONDITION_BULB_TIMER) ||
	    !conditions->bulb_timer_seconds ||
	    !conditions->bulb_timer_denominator)
		return PENTAX_BULB_PROBE_TIMER_UNAVAILABLE;
	if (conditions->bulb_timer_seconds > conditions->bulb_timer_denominator)
		return PENTAX_BULB_PROBE_TIMER_TOO_LONG;
	return PENTAX_BULB_PROBE_SAFE;
}
