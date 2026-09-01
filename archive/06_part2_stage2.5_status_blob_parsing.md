# STAGE 2.5: Status Blob Parsing (`GetAllConditions` 0x900F)

> **OBSOLETE:** Offsets and layouts are not sufficiently verified here. Use P1 and P7 in [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md).
**Goal**: Parse the `GetAllConditions` response to provide status feedback.

**Files to Modify**:
- `ptp2/ptp.c` – In the handler for 0x900F, instead of just emitting raw data, we will parse it and emit meaningful status information via libgphoto2's status reporting mechanism (or we can store the parsed data in the camera's private data and provide getters for status properties).
- We may need to add new status properties to libgphoto2 (e.g., battery level, storage state, etc.) or use existing ones.

**Approach**:
Rather than emitting raw data, we will parse the blob and update the camera's private data (if we have a camera struct) or we can emit the parsed data as separate properties via the standard PTP property system? However, the status blob is not a standard PTP property; it's a vendor-specific opcode.

Simpler: In the handler for 0x900F, we parse the blob and store the relevant status fields in the camera's private data (if we extend the camera struct). Then we can provide getters for these status fields as standard PTP properties (e.g., we could map battery level to the standard battery level property if it makes sense, or we create new vendor-specific status properties).

Given the complexity, we can start by simply emitting the raw data and letting the host application parse it (if the host application is modified). But for a generic libgphoto2 improvement, we want to make the status available via standard properties.

We will add new properties to `ptp2/config.c` for the status fields we want to expose, and in their getters, we will compute the value from the last cached `GetAllConditions` blob.

**Exact Changes**:
1. Add a struct to hold the last `GetAllConditions` blob (or parsed fields) in the camera's private data (if we have one) or in a global per-port variable.
2. In the handler for 0x900F, store the blob (or parse and store fields).
3. Add getters for status properties (e.g., battery level, storage state, etc.) that read from the cached data.
4. Add these properties to `ptp2/config.c` with appropriate property codes (we can use standard PTP property codes where applicable, or define new vendor-specific status property codes if needed).

Given the time, we will outline the approach:

- In `ptp2/ptp.c`, add a static struct `LastGetAllConditions` per port (or per camera) to hold the last blob or parsed values.
- In the handler for 0x900F, parse the blob and fill this struct.
- Add property getters for:
  - Battery level (if available in blob)
  - Storage state (sd1CardState, sd2CardState)
  - Remaining space (sd1Remain, sd2Remain)
  - Capture mode info
  - Mode enable info
  - Astrotracer phase/state/time limit
  - Pixel shift status (read-only indicator)
  - Current writing slot
  - etc.

We will map these to existing libgphoto2 properties where possible (e.g., battery level maps to GP_CAPTURE_BATTERY_LEVEL, storage state may not have a direct equivalent, we may need to use generic integer properties).

**Exit Criteria for Stage 2.5**:
- `GetAllConditions` (0x900F) handler implemented.
- Last blob parsed and stored.
- At least 80% of the documented status fields are available as gettable properties (standard or vendor-specific).
- Values update correctly when the blob changes.
- Pixel shift status correctly reported as read-only indicator.
- Astrotracer phase/state correctly reported.
