#ifndef PENTAX_BULB_TIMED_PROBE_GUARD_H
#define PENTAX_BULB_TIMED_PROBE_GUARD_H

#include <stdint.h>
#include "pentax-utils.h"

typedef enum {
	PENTAX_BULB_PROBE_SAFE = 0,
	PENTAX_BULB_PROBE_WRONG_CAMERA,
	PENTAX_BULB_PROBE_NOT_READY,
	PENTAX_BULB_PROBE_BUSY,
	PENTAX_BULB_PROBE_WRONG_MODE,
	PENTAX_BULB_PROBE_TIMER_UNAVAILABLE,
	PENTAX_BULB_PROBE_TIMER_TOO_LONG
} PentaxBulbProbeGuardResult;

PentaxBulbProbeGuardResult pentax_bulb_probe_validate_camera (
	uint16_t usb_vendor, uint16_t usb_product, uint32_t model_no,
	int supported_model, int vendor_mode_enabled);
PentaxBulbProbeGuardResult pentax_bulb_probe_validate_conditions (
	const PentaxConditions *conditions);

#endif
