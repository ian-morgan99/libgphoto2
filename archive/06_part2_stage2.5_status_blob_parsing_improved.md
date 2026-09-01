# STAGE 2.5: Status Blob Parsing (IMPROVED)

> **OBSOLETE:** Despite its filename, its layouts require trace validation. Use P1 and P7 in [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md).
**Goal**: Implement status blob parsing with validated fields based on actual hardware captures.

**Files to Modify**:
- `ptp2/ptp.c` – Add status blob handling and parsing functions.
- `ptp2/library.c` – Add status polling function and integrate with camera status reporting.
- `ptp2/config.c` – Add status properties using real `struct menu`/`struct submenu` format.
- `ptp2/ptp.h` – Add any needed status-related definitions.

**Exact Changes**:

## Status Blob Implementation Approach

Based on hardware captures and source analysis, implement a robust status blob parser with:

1. **In `ptp2/ptp.c`**:
   - Add wrapper function for `GetAllConditions` (0x900F) that:
     * Checks vendor mode is enabled
     * Calls `ptp_pentax_get_all_conditions()` to get raw blob
     * Parses blob using validated offsets and formats
     * Stores parsed status in camera's private data (or per-port state)
   - Implement bounds-checked helper functions for parsing:
     ```c
     static int
     pentax_blob_get_u8(const unsigned char *blob, int length, int offset, uint8_t *value) {
         if (offset + 1 > length)
             return -1;
         *value = blob[offset];
         return 0;
     }
     
     static int
     pentax_blob_get_u16(const unsigned char *blob, int length, int offset, uint16_t *value) {
         if (offset + 2 > length)
             return -1;
         *value = (blob[offset] |
                   (blob[offset+1] << 8));
         return 0;
     }
     
     static int
     pentax_blob_get_u32(const unsigned char *blob, int length, int offset, uint32_t *value) {
         if (offset + 4 > length)
             return -1;
         *value = (blob[offset] |
                   (blob[offset+1] << 8) |
                   (blob[offset+2] << 16) |
                   (blob[offset+3] << 24));
         return 0;
     }
     
     static int
     pentax_blob_get_s32(const unsigned char *blob, int length, int offset, int32_t *value) {
         if (offset + 4 > length)
             return -1;
         *value = (int32_t)(blob[offset] |
                           (blob[offset+1] << 8) |
                           (blob[offset+2] << 16) |
                           (blob[offset+3] << 24));
         return 0;
     }
     ```
   - Add function to update and cache status:
     ```c
     uint16_t
     ptp_pentax_update_and_cache_status(PTPParams *params)
     {
         unsigned char *blob = NULL;
         unsigned int blob_size = 0;
         uint16_t ret, ret2;
         
         ret = ptp_pentax_get_all_conditions(params, &blob, &blob_size);
         if (ret != PTP_RC_OK)
             return ret;
         
         /* Parse and store status fields */
         if (params->pentax_state) {
             /* Battery level (example offset - to be validated) */
             pentax_blob_get_u8(blob, blob_size, PENTAX_STATUS_BATTERY_LEVEL_OFFSET, 
                               &params->pentax_state->battery_level);
            
             /* Capture state */
             pentax_blob_get_u8(blob, blob_size, PENTAX_STATUS_CAPTURE_STATE_OFFSET,
                               &params->pentax_state->capture_state);
            
             /* Storage states */
             pentax_blob_get_u8(blob, blob_size, PENTAX_STATUS_SD1_STATE_OFFSET,
                               &params->pentax_state->sd1_state);
             pentax_blob_get_u8(blob, blob_size, PENTAX_STATUS_SD2_STATE_OFFSET,
                               &params->pentax_state->sd2_state);
            
             /* Remaining space (example - may need different handling) */
             pentax_blob_get_u32(blob, blob_size, PENTAX_STATUS_SD1_REMAIN_OFFSET,
                               &params->pentax_state->sd1_remain);
             pentax_blob_get_u32(blob, blob_size, PENTAX_STATUS_SD2_REMAIN_OFFSET,
                               &params->pentax_state->sd2_remain);
            
             /* Current writing slot */
             pentax_blob_get_u8(blob, blob_size, PENTAX_STATUS_CURRENT_SLOT_OFFSET,
                               &params->pentax_state->current_slot);
            
             /* Add more fields as validated */
         }
         
         free(blob);
         return PTP_RC_OK;
     }
     ```

2. **In `ptp2/library.c`**:
   - Add status polling function that:
     * Is called periodically (e.g., every 5 seconds) or on demand
     * Calls `ptp_pentax_update_and_cache_status()`
     * Updates libgphoto2 camera status fields
   - Integrate with existing status reporting mechanisms
   - Add getter functions for status properties that read from cached data

3. **In `ptp2/config.c`**:
   - Add status properties using real `struct menu`/`struct submenu` format
   - Map to appropriate libgphoto2 properties where possible:
     * Battery level → GP_CAPTURE_BATTERY_LEVEL
     * Storage state → May need custom properties or generic integers
     * Remaining space → Generic integer properties
     * Current slot → Custom or generic integer
   - Example:
     ```c
     static const struct _CamPropDesc pentax_status_props[] = {
         {
             .label = "batterylevel",
             .prop = 0x5001, /* Example vendor status property code */
             .type = GP_CAPTURE_BATTERY_LEVEL,
             .get = pentax_get_battery_level,
             .set = NULL, /* read-only */
             .min = 0,
             .max = 100,
             .step = 1
         },
         /* ... more status properties ... */
     };
     ```

## Initial Validated Status Fields to Implement

Start with these fields (validate offsets with hardware captures):

| Field | Description | Suggested Offset | Type | Libgphoto2 Mapping |
|-------|-------------|------------------|------|-------------------|
| Battery Level | 0-100% | To be validated | uint8_t | GP_CAPTURE_BATTERY_LEVEL |
| Capture State | 0=idle, 1=capturing, etc. | To be validated | uint8_t | Generic integer or custom |
| SD1 Card State | 0=none, 1=ok, 2=error, etc. | To be validated | uint8_t | Generic integer |
| SD2 Card State | 0=none, 1=ok, 2=error, etc. | To be validated | uint8_t | Generic integer |
| SD1 Remaining | Number of pictures | To be validated | uint32_t | Generic integer |
| SD2 Remaining | Number of pictures | To be validated | uint32_t | Generic integer |
| Current Slot | 0=SD1, 1=SD2 | To be validated | uint8_t | Generic integer |
| AeroTracer State | 0=off, 1=on, etc. | To be validated | uint8_t | Generic integer |
| Pixel Shift Status | 0=off, 1=on, 2=error | To be validated | uint8_t | Generic integer (read-only) |

## Validation Requirements

For each status field implementation:

1. **Offset Validation**:
   - Correlate blob changes with known camera state changes
   - Verify offset consistency across firmware versions/models
   - Confirm little-endian decoding is correct

2. **Value Validation**:
   - Battery level: Compare with camera LCD battery indicator
   - Capture state: 0 when idle, changes during capture/transfer
   - Storage states: Match card insertion/removal and error states
   - Remaining space: Decreases as pictures are taken, increases after deletion
   - Current slot: Toggles when slot changed via camera menu
   - AeroTracer: Changes when feature enabled/disabled
   - Pixel shift: Read-only indicator of feature availability

3. **Timing Validation**:
   - Status updates within reasonable time of state change
   - No excessive polling that interferes with other operations
   - Graceful handling of temporary blob unavailability

## Specific Implementation Notes

### Handling Blob Length Variations
- Different firmware versions may have different blob lengths
- Parser must validate minimum length before accessing offsets
- Consider implementing version-specific offset tables if needed
- Log warnings when blob is shorter than expected but continue with available data

### Endianness and Data Formats
- All multi-byte values are little-endian (confirmed from source)
- Some fields may be packed or require scaling (validate per field)
- Boolean fields may use 0/1 or other conventions

### Thread Safety and Concurrency
- Status updates should be atomic or use appropriate locking
- Consider rate-limiting status polls to avoid overwhelming camera
- Cache status with timestamp to avoid excessive polling

**Exit Criteria**:
- Status blob wrapper function implemented in `ptp2/ptp.c`
- Bounds-checked helper functions for blob parsing implemented
- Status caching mechanism stores parsed fields from blob
- At least 6 validated status fields are available as gettable properties:
  * Battery level
  * Capture state
  * SD1 and SD2 card states
  * SD1 and SD2 remaining space
  * Current writing slot
- Values update correctly when camera state changes
- Parser handles blob length variations safely
- Implementation follows libgphoto2 conventions for status reporting
- Status properties are readable via standard libgphoto2 interfaces (e.g., `gphoto2 --summary`)
