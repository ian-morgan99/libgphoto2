# STAGE 2.2: Pentax-Specific Property Support (IMPROVED)

> **OBSOLETE:** Despite its filename, this is not canonical. Use P1 and P6 in [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md).
**Goal**: Add Pentax vendor property codes (0xD0xx range) to `ptp2/config.c` using actual device descriptors.

**Files to Modify**:
- `ptp2/config.c` – Add entries for each Pentax vendor property using real `struct menu`/`struct submenu` format.
- `ptp2/library.c` – Implement getter/setter functions for each property.
- `ptp2/mtp.h` – Add property code definitions if missing.
- `ptp2/ptp.h` – Add any needed PTP property code definitions.

**Exact Changes**:

## Property Implementation Approach

For each Pentax vendor property discovered via `GetDevicePropDesc`:

1. **In `ptp2/library.c`**:
   - Implement typed getter function that:
     * Checks vendor mode is enabled
     * Calls `ptp_get_device_prop_value()` with correct property code and datatype
     * Converts raw PTP value to libgphoto2 format (string, enum, integer) per property specification
     * Handles endianness and scaling as needed
   - Implement typed setter function (if writable) that:
     * Checks vendor mode is enabled
     * Converts libgphoto2 value to raw PTP format per property specification
     * Handles endianness and scaling as needed
     * Calls `ptp_set_device_prop_value()` with correct property code and datatype

2. **In `ptp2/config.c`**:
   - Add entry using the real `struct menu`/`struct submenu` format from `config.c`
   - Example structure to follow (adapt from existing entries):
     ```c
     static const struct _CamPropDesc pentax_props[] = {
         {
             .label = "shutterspeed",
             .prop = 0xD00F,
             .type = GP_CAPTURE_SHUTTER_SPEED,
             .get = pentax_get_shutterspeed,
             .set = pentax_set_shutterspeed, /* NULL if read-only */
             .min = ...,
             .max = ...,
             .step = ...,
             .readonly = ... /* boolean */
         },
         /* ... more properties ... */
     };
     ```
   - Add the property set to the appropriate camera section in the properties initialization

## Initial Property Priority Implementation

Implement these properties FIRST (based on common usage and analysis):

| Property Code | Property Name | libgphoto2 Mapping | Notes |
|---------------|---------------|-------------------|-------|
| 0xD00F | Shutter Speed | GP_CAPTURE_SHUTTER_SPEED | Rational or packed format - needs conversion |
| 0xD005 | ISO Speed | GP_CAPTURE_ISO_SPEED | Usually UINT16 enumeration |
| 0xD015 | Aperture | GP_CAPTURE_APERTURE | Rational or packed format |
| 0xD018 | White Balance Temperature | GP_CAPTURE_WHITE_BALANCE_TEMPERATURE | UINT16 in Kelvin |
| 0xD014 | Focus Mode | GP_CAPTURE_FOCUS_MODE | Enumeration (AF-S, AF-C, MF, etc.) |
| 0xD00E | Exposure Compensation | GP_CAPTURE_EXPOSURE_COMPENSation | Signed rational or integer |
| 0xD01D | Drive Mode | GP_CAPTURE_DRIVE_MODE | Enumeration (single, continuous, timer, etc.) |
| 0xD020 | File Format | GP_CAPTURE_IMAGE_FORMAT | Enumeration (JPEG, RAW, etc.) |

## Property Validation Requirements

For each property implementation:

1. **Getter Validation**:
   - Compare returned value against camera LCD display
   - Test full range of values camera supports
   - Verify read-only properties cannot be changed via setter
   - Confirm correct data type handling (endianness, scaling)

2. **Setter Validation**:
   - Test setting values and verify they take effect (check LCD or subsequent getter)
   - Test boundary values (min, max, invalid)
   - Verify camera rejects invalid values with appropriate error
   - Confirm setting persists until changed or camera power cycle

## Specific Implementation Examples

### Shutter Speed (0xD00F) - Rational Format Example
```c
/* In ptp2/library.c */
static uint16_t
pentax_get_shutterspeed(Camera *camera, void *value, ...) {
    PTPParams *params = (PTPParams *) camera->private_data;
    uint32_t raw_val;
    uint16_t ret;
    int numerator, denominator;
    
    ret = ptp_get_device_prop_value(params, PTP_DPC_PENTAX_ShutterSpeed, &raw_val, sizeof(raw_val));
    if (ret != PTP_RC_OK)
        return ret;
    
    /* Convert PTP rational to shutter speed string */
    /* Format: numerator in high 16 bits, denominator in low 16 bits */
    numerator = (raw_val >> 16) & 0xFFFF;
    denominator = raw_val & 0xFFFF;
    
    if (denominator == 0)
        return PTP_RC_Error;
        
    /* Convert to string like "1/125" */
    char *buf = malloc(16);
    if (!buf)
        return PTP_RC_Error;
        
    if (numerator == 1)
        snprintf(buf, 16, "1/%d", denominator);
    else
        snprintf(buf, 16, "%d/%d", numerator, denominator);
        
    *((char **)value) = buf;
    return PTP_RC_OK;
}

/* In ptp2/config.c */
{
    .label = "shutterspeed",
    .prop = 0xD00F,
    .type = GP_CAPTURE_SHUTTER_SPEED,
    .get = pentax_get_shutterspeed,
    .set = pentax_set_shutterspeed,
    .readonly = 0
}
```

### ISO Speed (0xD005) - Enumeration Example
```c
/* In ptp2/library.c */
static const struct _Value iso_speed_values[] = {
    { 100, "100" },
    { 200, "200" },
    { 400, "400" },
    { 800, "800" },
    { 1600, "1600" },
    { 3200, "3200" },
    { 6400, "6400" },
    { 12800, "12800" },
    { 25600, "25600" },
    { 51200, "51200" },
    { 0, NULL }
};

static uint16_t
pentax_get_isospeed(Camera *camera, void *value, ...) {
    PTPParams *params = (PTPParams *) camera->private_data;
    uint16_t raw_val;
    uint16_t ret;
    
    ret = ptp_get_device_prop_value(params, PTP_DPC_PENTAX_ISOSpeed, &raw_val, sizeof(raw_val));
    if (ret != PTP_RC_OK)
        return ret;
    
    *((int *)value) = raw_val;
    return PTP_RC_OK;
}

static uint16_t
pentax_set_isospeed(Camera *camera, const void *value, ...) {
    PTPParams *params = (PTPParams *) camera->private_data;
    int iso = *((int *)value);
    uint16_t ret;
    
    ret = ptp_set_device_prop_value(params, PTP_DPC_PENTAX_ISOSpeed, &iso, sizeof(iso));
    return ret;
}

/* In ptp2/config.c */
{
    .label = "iso",
    .prop = 0xD005,
    .type = GP_CAPTURE_ISO_SPEED,
    .get = pentax_get_isospeed,
    .set = pentax_set_isospeed,
    .min = 100,
    .max = 51200,
    .step = 0, /* enumeration - use value table */
    .value = iso_speed_values,
    .readonly = 0
}
```

**Exit Criteria**:
- All Pentax vendor property codes from the analysis have working getters/setters in `ptp2/library.c`
- Properties are correctly mapped in `ptp2/config.c` using the real `struct menu`/`struct submenu` format
- Getters return values that match camera LCD display
- Setters successfully change camera settings when writable
- Properties are gated by vendor mode enabled check
- Implementation handles correct data types, endianness, and scaling per property specification
- At least the 8 priority properties listed above are fully implemented and validated
