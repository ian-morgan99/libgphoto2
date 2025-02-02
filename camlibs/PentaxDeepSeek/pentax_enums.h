#ifndef _PENTAX_ENUMS_H_
#define _PENTAX_ENUMS_H_

typedef enum {
    SHOOTING_MODE_NORMAL,
    SHOOTING_MODE_BULB,
    SHOOTING_MODE_MIRROR_UP,
    SHOOTING_MODE_INTERVAL,
    SHOOTING_MODE_BRACKETING,
    SHOOTING_MODE_SELF_TIMER
} ShootingModeEnum;

typedef enum {
    FOCUS_MODE_AUTO,
    FOCUS_MODE_MANUAL
} FocusModeEnum;

typedef enum {
    METERING_MODE_MATRIX,
    METERING_MODE_CENTER_WEIGHTED,
    METERING_MODE_SPOT
} MeteringModeEnum;

#endif // _PENTAX_ENUMS_H_
