# STAGE 2.2: Pentax-Specific Property Support

> **OBSOLETE:** Historical draft only. Use P1 and P6 in [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md).
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
