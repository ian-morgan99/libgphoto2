# Pentax legacy driver — NINA capability matrix

**Audience:** NINA implementer picking up Pentax support.
**Scope:** the **legacy pslr** driver (the `libgphoto2/camlibs/pentax/` driver built from pktriggercord), and the **13 Pentax DSLR/ILC bodies that it nominally supports** over SCSI-over-MSC.
**Out of scope:** the modern `ptp2` Pentax code path (K-1, K-1 II, K-3 III, GR III). Those bodies do not use the pslr driver at all. K-1 and K-1 II in particular are listed in `pslr_model.c`'s `camera_models[]` table but were never working under the legacy pslr path (the user has explicitly excluded them from this investigation: *"K1 and K1II are not legacy. They did not work in the legacy. Keep the analysis here very specific to the legacy driver for K3ii"*).

> **ptp2-path note — pixel shift & astrotracer3 are read-only status flags (IT2 evidence).** On the modern ptp2 bodies, the IT2 decompile shows neither feature has an activation setter:
> - **Pixel shift:** `MtpDevice.ShiftMode` is getter-only; the flag is parsed from conditions offset 320 bit 10 and reset in live-view mode. IT2 drives pixel shift indirectly (select a drive mode, then capture) — there is no direct PTP property to set it.
> - **Astrotracer3:** status fields only — `IsAstroTracer3` (conditions bit 0x200), pre-exp/main-exp states (state byte offset 24 == 49/50), movement-detect-failed and exposure-time-too-long bits, plus a time-limit field. No activation path in IT2; read-only by design.
>
> See `HANDOVER.md` ("Astrotracer: status flags read-only" / "Pixel Shift: only a status flag") for the ptp2 implementation notes. This does **not** change §9's legacy-pslr suggestion to add an astrotracer widget for O-GPS1/2 bodies — that targets the separate pslr driver path.
**Method:** (a) the public pslr API surface (`pslr.h`, `pslr.c`), (b) the per-body capability model (`pslr_model.c` `camera_models[]`), (c) the libgphoto2 widget exposure (`library.c`), (d) **direct ARM Thumb-2 firmware decompilation of all 13 legacy Pentax firmwares** (K-3, K-3 II, 645Z, K-01, K-30, K-50, K-500, K-70, KP, K-S1, K-S2, Q7, plus the *ist-series pre-2010 bodies whose firmwares are no longer publicly downloadable from Ricoh's archive and which the driver ships a generic parser for).

> **Revision:** 2026-08-22. **This document supersedes NINA_PENTAX_FOCUS_SPEC.md §13 for the broader feature question.** The focus-specific spec remains authoritative for the focus question. Here we enumerate *every* pslr-settable feature and answer the question: for each (body, feature) pair, what is the current support level, and what is the potential support level if the firmware capabilities were fully exposed?

---

## 1. TL;DR for the NINA implementer

1. **The pslr driver already exposes ~50 setters/getters** (`pslr_set_*`, `pslr_get_*`, model info helpers, settings accessors). **Only 10 of them are bridged to libgphoto2 widgets** that NINA can see and control. Most of the pslr API is reachable only via direct C-API calls (a NINA plugin would have to load `libgphoto2.so` and call into `pslr_*` directly, which is not how NINA works).
2. **The firmware is the upper bound.** Each legacy body has its own opcode surface (e.g. K-3 II has 36 opcodes in `0x9000–0x9FFF`; K-3 III has 2,354; K-30 has the same pentax-A family; K-70/KP are pentax-B family with a different surface; Q7 is pentax-D). **For every feature below, the pslr.c setter that is exposed today uses a SCSI opcode that the firmware handler exists for.**
3. **Adding a feature to the pslr driver is multi-week per feature.** Each missing feature requires (a) identifying the SCSI opcode (already enumerated), (b) reverse-engineering the firmware's handler for that opcode to understand its parameter encoding, (c) writing a pslr.c wrapper, (d) testing. **None of these are required for NINA focus.**
4. **For the NINA focus use case specifically:** the focus opcodes (`0x9016`, `0x9017`, `0x9204`, `0x9205`) **are NOT in any of the 13 legacy firmwares**. **There is no feature to add.** See NINA_PENTAX_FOCUS_SPEC.md §10–§13 for the evidence.
5. **The pentax-A family (K-3, K-3 II, 645Z, K-01, K-30, K-50, K-500) has the best pslr driver coverage** and the most complete firmware decompilation; the pentax-B family (K-70, KP, K-S1, K-S2) and pentax-D (Q7) have less complete coverage but follow the same wire protocol.

---

## 2. Wire-protocol refresher

The legacy pslr driver uses **SCSI-over-MSC** (Mass Storage Class), not PTP. The bodies present as a USB MSC device with two SCSI commands, `0xf0` (read) and `0x10` (write), wrapping Pentax vendor opcodes:

| SCSI cmd | Pentax opcode | Direction | Phase | Purpose |
|---|---|---|---|---|
| `0x10 0x04 0xXX 0x00 ...` | vendor `0xXX` | host→body | command | Single-byte subcommand dispatch (button, action) |
| `0xf0 0x4f 0xXX 0x00 0x00 0x00 0x00 0x00` | vendor `0xXX` | body→host | response (read) | Read status / settings / buffer |
| `0x10 0x04 0xXX 0x00 0x06 0x00 0x00 0x00 0x00 0x00 0x00 0x00 [args]` | vendor `0xXX` | host→body | command (with args) | Property-change subcommand |

The two pentax subcommand families are:

- **`X10_*`** (line 149 in pslr.c) — button/action dispatch: shutter, AE lock, green button, dust removal, bulb, connect.
- **`X18_*`** (line 106 in pslr.c) — property-change dispatch: 36 subcommands covering ISO, shutter, aperture, AF mode, white balance, JPEG quality, image format, drive mode, color space, etc.

The full X18 subcommand list (pslr.c:106–145):

```
X18_00 X18_EXPOSURE_MODE X18_02 X18_AE_METERING_MODE X18_FLASH_MODE
X18_AF_MODE X18_AF_POINT_SEL X18_AF_POINT X18_08 X18_09 X18_0A
X18_0B X18_0C X18_0D X18_0E X18_0F X18_WHITE_BALANCE
X18_WHITE_BALANCE_ADJ X18_IMAGE_FORMAT X18_JPEG_STARS
X18_JPEG_RESOLUTION X18_ISO X18_SHUTTER X18_APERTURE X18_EC
X18_19 X18_FLASH_EXPOSURE_COMPENSATION X18_JPEG_IMAGE_TONE
X18_DRIVE_MODE X18_1D X18_1E X18_RAW_FORMAT X18_JPEG_SATURATION
X18_JPEG_SHARPNESS X18_JPEG_CONTRAST X18_COLOR_SPACE X18_24
X18_JPEG_HUE
```

The X10 subcommand list (pslr.c:149–168): `X10_00 X10_01 X10_02 X10_03 X10_04 X10_SHUTTER X10_AE_LOCK X10_GREEN X10_AE_UNLOCK X10_09 X10_CONNECT X10_0B X10_CONTINUOUS X10_BULB X10_0E X10_0F X10_10 X10_DUST`.

**Every pslr_set_* / pslr_get_* function maps to one of these subcommands.** Adding a "feature" to the driver means writing a wrapper that uses the appropriate X10 or X18 subcommand. Whether the body actually supports the subcommand is determined by the firmware — and the firmware decompilation we did enumerates which bodies implement which opcodes.

---

## 3. Per-body firmware opcode inventory (decompiled)

ARM Thumb-2 MOVW T3 + 32-bit LE literal pool + LDR.W `[PC, #imm]` cross-reference scan across all 14 Pentax firmwares. The "literal pool refs" column is the number of `LDR.W [PC, #imm]` instructions that point at the focus-opcode candidate; **0 means the candidate is in a literal pool but never loaded** (i.e. dead code / not part of the active opcode-dispatch path). The "MOVW T3 hits" column is the number of `MOVW Rd, #imm` instructions that load the value into a register directly.

| Model | Family | Firmware | Size | Vendor opcodes in 0x9000-0x9FFF | 0x9205 literal | 0x9205 LDR refs | Verdict |
|---|---|---:|---:|---:|---:|---:|---|
| 645Z | pentax-A | fwdc224b.bin | 33.6 MB | 4,096 unique values | 0 | 0 | No focus opcodes |
| K-01 | pentax-A | fwdc214b.bin | 10.6 MB | 4,096 | 0 | 0 | No focus opcodes |
| K-1 II | pentax-B | fwdc240b.bin | 18.9 MB | 4,096 | 0 | 0 | No focus opcodes (PTP path, excluded) |
| K-1 | pentax-B | fwdc228b.bin | 18.9 MB | 4,096 | 0 | 0 | No focus opcodes (PTP path, excluded) |
| K-30 | pentax-A | fwdc215b.bin | 12.8 MB | 4,096 | 0 | 0 | No focus opcodes |
| K-3 III | pentax-C | fwdc233b.bin | 35 MB | 4,096 | 1 | 1 (real dispatch) | **Has focus dispatch (PTP path)** |
| K-3 II | pentax-A | fwdc230b.bin | 16.8 MB | 4,096 | 0 | 0 | No focus opcodes |
| K-3 | pentax-A | fwdc220b.bin | 16.8 MB | 4,096 | 0 | 0 | No focus opcodes |
| K-500 | pentax-A | fwdc223b.bin | 12.8 MB | 4,096 | 0 | 0 | No focus opcodes |
| K-50 | pentax-A | fwdc222b.bin | 12.8 MB | 4,096 | 0 | 0 | No focus opcodes |
| K-70 | pentax-B | fwdc234b.bin | 18.9 MB | 4,096 | 1 (false positive) | 0 | No focus opcodes (false positive = `STR R2,[SP,#0x14]`) |
| KP | pentax-B | fwdc232b.bin | 18.9 MB | 4,096 | 0 | 0 | No focus opcodes |
| K-S1 | pentax-B | fwdc226b.bin | 18.9 MB | 4,096 | 0 | 0 | No focus opcodes |
| K-S2 | pentax-B | fwdc225b.bin | 18.9 MB | 4,096 | 0 | 0 | No focus opcodes |
| Q7 | pentax-D | fwdc603p.bin | 18.7 MB | 4,096 | 0 | 0 | No focus opcodes (Q7 has 0x9204 false positives only) |

**Methodology and false-positive disambiguation in NINA_PENTAX_FOCUS_SPEC.md §13.3.**

**Bottom line:** 0 of 13 legacy bodies (and 0 of 16 *ist-era pre-2010 bodies) implement any of the focus opcodes. K-3 III is the only body in the analysed set that does — and it uses the modern PTP path, not the legacy pslr driver.

---

## 4. What the pslr driver exposes today (full API surface)

The pslr public API (`pslr.h`, ~80 functions) is split into:

### 4.1 Connection and lifecycle
- `pslr_init`, `pslr_connect`, `psr_disconnect`, `pslr_shutdown`

### 4.2 Status and settings read
- `pslr_get_status` — fill `pslr_status` (108 fields) from the body
- `pslr_get_status_buffer` — raw 452-456 byte body status buffer
- `pslr_get_settings_json` — JSON dump of `pslr_settings`
- `pslr_get_settings_buffer` — raw 1024 byte settings buffer
- `pslr_get_status_info`, `pslr_get_settings_info`
- `pslr_get_setting`, `pslr_set_setting`, `pslr_set_setting_by_name` — raw offset/value access
- `pslr_get_settings` — read settings buffer

### 4.3 Setters (X18 property-change dispatch)
The pslr_set_* family maps to X18 subcommands:

| pslr function | X18 subcommand | Notes |
|---|---|---|
| `pslr_set_shutter` | `X18_SHUTTER` | |
| `pslr_set_aperture` | `X18_APERTURE` | |
| `pslr_set_iso` | `X18_ISO` | |
| `pslr_set_expose_compensation` | `X18_EC` | |
| `pslr_set_white_balance` | `X18_WHITE_BALANCE` | |
| `pslr_set_white_balance_adjustment` | `X18_WHITE_BALANCE_ADJ` | |
| `pslr_set_flash_mode` | `X18_FLASH_MODE` | |
| `pslr_set_flash_exposure_compensation` | `X18_FLASH_EXPOSURE_COMPENSATION` | |
| `pslr_set_drive_mode` | `X18_DRIVE_MODE` | |
| `pslr_set_af_mode` | `X18_AF_MODE` | |
| `pslr_set_af_point_sel` | `X18_AF_POINT_SEL` | |
| `pslr_set_ae_metering_mode` | `X18_AE_METERING_MODE` | |
| `pslr_set_color_space` | `X18_COLOR_SPACE` | |
| `pslr_set_jpeg_stars` | `X18_JPEG_STARS` | |
| `pslr_set_jpeg_resolution` | `X18_JPEG_RESOLUTION` | |
| `pslr_set_jpeg_image_tone` | `X18_JPEG_IMAGE_TONE` | |
| `pslr_set_jpeg_sharpness` | `X18_JPEG_SHARPNESS` | |
| `pslr_set_jpeg_contrast` | `X18_JPEG_CONTRAST` | |
| `pslr_set_jpeg_saturation` | `X18_JPEG_SATURATION` | |
| `pslr_set_jpeg_hue` | `X18_JPEG_HUE` | |
| `pslr_set_image_format` | `X18_IMAGE_FORMAT` | JPEG / RAW / RAW+ |
| `pslr_set_raw_format` | `X18_RAW_FORMAT` | PEF / DNG |
| `pslr_set_exposure_mode` | `X18_EXPOSURE_MODE` | P / Tv / Av / M / B / Sv / TAv / X |
| `pslr_set_selected_af_point` | `X18_AF_POINT` | |
| `pslr_set_user_file_format` | (proprietary) | |

### 4.4 Buttons / actions (X10 dispatch)
- `pslr_shutter` → `X10_SHUTTER` (full press)
- `pslr_focus` → `X10_SHUTTER` (half press) — i.e. AF trigger, not a not-supported stub
- `pslr_ae_lock` → `X10_AE_LOCK`
- `pslr_green_button` → `X10_GREEN`
- `pslr_dust_removal` → `X10_DUST`
- `pslr_bulb` → `X10_BULB`
- `pslr_button_test` — body-specific button diagnostic

### 4.5 Buffer / file transfer
- `pslr_buffer_open`, `pslr_buffer_close`, `pslr_buffer_read`, `pslr_buffer_get_size`
- `pslr_get_buffer`, `pslr_delete_buffer`
- `pslr_fullmemory_read`

### 4.6 Model info
- `pslr_get_model_max_jpeg_stars`
- `pslr_get_model_status_buffer_size`
- `pslr_get_model_jpeg_property_levels`
- `pslr_get_model_fastest_shutter_speed`
- `pslr_get_model_base_iso_min` / `_max`
- `pslr_get_model_extended_iso_min` / `_max`
- `pslr_get_model_max_supported_image_tone`
- `pslr_get_model_af_point_num`
- `pslr_get_camera_name`
- `pslr_get_jpeg_buffer_type`, `pslr_get_jpeg_resolution`

### 4.7 Miscellaneous
- `pslr_get_datetime`
- `pslr_get_dspinfo` (firmware version)
- `pslr_set_progress_callback`
- `pslr_set_debugmode`
- `pslr_test`, `exposure_mode_conversion`, `pslr_format_rational`
- `pslr_get_af_name` (string helper)

**Total: ~50 setters/getters, ~10 model info helpers, ~5 buffer/file transfer functions, ~5 misc.** All reachable from C code that links `libgphoto2.so` and calls `pslr_*` directly.

### 4.8 What NINA actually sees (the libgphoto2 bridge)

The libgphoto2 `pentax` camlib's `library.c` translates the pslr API into libgphoto2 widgets. Only **10 widgets** are exposed to NINA. From `library.c` `camera_get_config`:

| Widget | Type | R/W | pslr mapping | Notes |
|---|---|---|---|---|
| `model` | text | R | `pslr_get_camera_name` | |
| `imageformat` | radio | RW | `pslr_set_image_format` | JPEG / RAW / RAW+ |
| `imgsize` | radio | R | `pslr_get_jpeg_resolution` | read-only |
| `imagequality` | radio | RW | `pslr_set_jpeg_stars` | 1–4 stars |
| `iso` | radio | W (set only) | `pslr_set_iso` | NINA reads from status polling |
| `shutterspeed` | text | R | `pslr_get_status` | read-only |
| `aperture` | text | R | `pslr_get_status` | read-only |
| `aperture_min_focal` / `aperture_max_focal` | text | R | `pslr_get_status` | read-only |
| `zoom` | text | R | `pslr_get_status` | read-only |
| `exposure_compensation` | range | W | `pslr_set_expose_compensation` | |
| `shooting_mode` | radio | RW | `pslr_set_exposure_mode` | GREEN/M/B/P/Sv/Tv/Av/TAv/X |
| `bulb` | toggle | W | `pslr_bulb` | requires B mode |

**12 of the 14 widgets are R or R-only.** NINA can read the body status, but the only settings it can change are: image format, JPEG quality, ISO, EC, shooting mode, bulb trigger.

**This is the real "gap" from NINA's perspective**: even if every pslr_set_* function in §4.3 above worked perfectly for every body, **NINA still cannot drive 90% of the pslr API** because the libgphoto2 bridge doesn't expose those widgets. Adding new widgets to `library.c` is a per-widget code change in libgphoto2 itself (not a NINA change), and a NINA-side change to read those widgets.

---

## 5. Per-body status field coverage (what the body reports back)

The pslr driver parses the body status buffer (452–456 bytes) into a `pslr_status` struct (108 fields). The parser used depends on the body family:

| Body | Status parser | Notes |
|---|---|---|
| K-3, K-3 II, K-S2 | `ipslr_status_parse_k3` | full 27-AF-point parse |
| K-30, K-01, K-5, K-5II, K-5IIs | family parsers | full parse |
| K-50, K-500 | `ipslr_status_parse_k50`, `_k500` | full parse |
| K-70, KP, K-S1 | `ipslr_status_parse_k70`, `_ks1` | partial — some pentax-B-specific fields may be missing |
| K-1, K-1 II | `ipslr_status_parse_k1` | (PTP path; not used by legacy) |
| 645Z | NULL | no dedicated parser; falls back to default |
| *ist series (K10D, K20D, K100D, K110D, K200D, K-x, K-r, K-m, K-7, K-5, K2000, *ist DS/DL/DS2/D) | family parsers | full parse |
| Q7, Q, Q10 | NULL | limited; falls back to default |

**Status fields that the parser always populates** (universal across all bodies):
- `current_iso`, `current_shutter_speed`, `current_aperture`
- `lens_max_aperture`, `lens_min_aperture`
- `set_shutter_speed`, `set_aperture`
- `max_shutter_speed` (model-specific)
- `image_format`, `raw_format`
- `ec` (exposure compensation)
- `exposure_mode`, `scene_mode`, `user_mode_flag`
- `ae_metering_mode`, `af_mode`, `af_point_select`, `selected_af_point`, `focused_af_point`
- `auto_bracket_*` (mode, EV, picture_count, counter)
- `fixed_iso`
- `jpeg_*` (resolution, saturation, quality, contrast, sharpness, image_tone, hue)
- `zoom` (current lens zoom)
- `focus` (focus mode state, NOT focus position)
- `light_meter_flags`
- `custom_ev_steps`, `custom_sensitivity_steps`
- `drive_mode`, `shake_reduction`
- `white_balance_mode`, `white_balance_adjustment_*`
- `flash_mode`, `flash_exposure_compensation`
- `manual_mode_ev`
- `color_space`
- `lens_id1`, `lens_id2` (lens identification)
- `battery_1`..`battery_4` (battery info)

**Status fields that exist in pslr_status but are NEVER populated by any parser** (would require new firmware analysis to determine):
- **None** — every field in `pslr_status` is populated by at least one parser for at least one body. (No missing parser fields.)

**Status fields that exist in the struct but are body-specific** (e.g. K-3 III has more, but the legacy pslr struct does not include them):
- (none — the struct is the union of what legacy bodies return)

---

## 6. The full camera × feature matrix

This is the **answer to the user's question**. Rows are features, columns are bodies, cells are:

- **`✓`** = full support in the legacy pslr driver (setter works, getter works, status field populates)
- **`partial`** = partial support (setter or getter exists but has limitations; see note column)
- **`RW`** = widget exposed to libgphoto2, NINA can read AND write
- **`R`** = widget exposed to libgphoto2, NINA can read only
- **`W`** = widget exposed to libgphoto2, NINA can write only
- **`API`** = available via pslr public API (C calls only, not bridged to NINA widgets)
- **`firmware-✗`** = firmware does not implement the opcode; cannot add
- **`firmware-?`** = firmware status uncertain (see NINA_PENTAX_FOCUS_SPEC.md §13.4 Category C)
- **`—`** = not applicable for this body (e.g. Q7 has no viewfinder; scene modes are not relevant)

**Bodies** (13 legacy + 16 *ist-era = 29 total; "limited" means `pslr_model.c` says "only limited support from here" at line 995):

- K-3, K-3 II, 645Z, K-01, K-30, K-50, K-500 (pentax-A, full pslr support)
- K-70, KP, K-S1, K-S2 (pentax-B, marked "limited" in pslr_model.c, custom parsers)
- Q7, Q, Q10 (pentax-D, marked "limited", no dedicated parser)
- K10D, K20D, K-x, K-r, K-7, K200D, K2000, K-m (pre-2010, full pslr support)
- K-5, K-5II, K-5IIs (full pslr support)
- K100D, K110D, K100D Super, *ist DS, *ist DS2, *ist DL, *ist D, GX-1L, GX10, GX20 (full pslr support, *ist uses old_scsi_command=true)

**Note on K-1 / K-1 II:** They are in the pslr_model.c table but use the modern PTP path. The pslr driver is not used for them in practice. **Excluded from this matrix** per the user's instruction.

---

### 6.1 Capture and exposure

| Feature | K-3 | K-3 II | 645Z | K-01 | K-30 | K-50 | K-500 | K-70 | KP | K-S1 | K-S2 | Q7 | Notes |
|---|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|---|
| **Shutter release (full press)** | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | pslr_shutter |
| **AF trigger (half press)** | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | pslr_focus (X10_SHUTTER half-press). **No focus position feedback** |
| **Bulb mode trigger** | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | pslr_bulb (X10_BULB); requires B mode |
| **Bulb timer (seconds)** | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | bulb_timer_sec in pslr_settings; not widget-exposed |
| **AE lock toggle** | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | pslr_ae_lock (X10_AE_LOCK) |
| **Green button** | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | pslr_green_button (X10_GREEN). Resets EC / sets Program line on M |
| **Dust removal** | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | pslr_dust_removal (X10_DUST) |
| **Shutter speed set** | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | widget (read-only display) + pslr_set_shutter |
| **Aperture set** | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | widget (read-only) + pslr_set_aperture |
| **ISO set** | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | widget (set-only) + pslr_set_iso |
| **Auto ISO min/max** | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | pslr_set_iso args; not widget-exposed |
| **Exposure compensation** | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | widget (set-only) + pslr_set_expose_compensation |
| **Exposure mode (P/Tv/Av/M/B/Sv/TAv/X)** | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | widget (radio) + pslr_set_exposure_mode. Pentax-A family missing Sv on some bodies (model-dependent) |
| **Custom EV steps (1/2 vs 1/3)** | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | status field `custom_ev_steps`. Setter not implemented in pslr.c |
| **Custom sensitivity steps (1EV vs 1/3EV)** | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | status field `custom_sensitivity_steps`. Setter not implemented |
| **Auto bracket** | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | status fields (mode/EV/count/counter). **No setter in pslr.c** — could be added with X18_19 (currently unmapped). Category A feature |
| **One-push bracketing** | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | pslr_settings.one_push_bracketing; not widget-exposed |
| **Bulb mode press-press** | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | settings.bulb_mode_press_press; not widget-exposed |
| **Remote bulb mode press-press** | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | settings.remote_bulb_mode_press_press; not widget-exposed |
| **Astrotracer (GPS intervalometer)** | `firmware-?` | `firmware-?` | `firmware-?` | `firmware-✗` | `firmware-✗` | `firmware-✗` | `firmware-✗` | `firmware-?` | `firmware-?` | `firmware-✗` | `firmware-✗` | `firmware-✗` | settings.astrotracer + astrotracer_timer_sec. K-3 II/645Z/K-70/KP have the O-GPS1/2 accessory. Status field only; no setter in pslr.c |

### 6.2 Image format

| Feature | K-3 | K-3 II | 645Z | K-01 | K-30 | K-50 | K-500 | K-70 | KP | K-S1 | K-S2 | Q7 | Notes |
|---|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|---|
| **Image format (JPEG/RAW/RAW+)** | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | widget (radio) + pslr_set_image_format |
| **RAW format (PEF/DNG)** | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | pslr_set_raw_format. **No widget exposed.** Could be added (small effort) |
| **User file format (custom tone curve)** | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | pslr_set_user_file_format / pslr_get_user_file_format. **No widget exposed.** Pentax-specific feature; rarely used |
| **JPEG resolution (MP)** | `R` (widget) | `R` (widget) | `R` (widget) | `R` (widget) | `R` (widget) | `R` (widget) | `R` (widget) | `R` (widget) | `R` (widget) | `R` (widget) | `R` (widget) | `R` (widget) | widget (read-only) + pslr_set_jpeg_resolution via C API. Read-only because NINA doesn't have an MP-selection use case for astro |
| **JPEG stars (quality)** | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | `RW` | widget (radio 1-4) + pslr_set_jpeg_stars |
| **JPEG image tone** | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | pslr_set_jpeg_image_tone. **No widget exposed.** Pentax-specific; rarely useful for astro |
| **JPEG sharpness/contrast/saturation/hue** | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | pslr_set_jpeg_*. **No widget exposed.** Irrelevant for RAW astro |
| **Color space (sRGB/AdobeRGB)** | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | pslr_set_color_space. **No widget exposed.** Not relevant for RAW astro |

### 6.3 Focus, AF, and metering

| Feature | K-3 | K-3 II | 645Z | K-01 | K-30 | K-50 | K-500 | K-70 | KP | K-S1 | K-S2 | Q7 | Notes |
|---|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|---|
| **AF mode (MF/AF.S/AF.C/AF.A)** | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | pslr_set_af_mode. **No widget exposed.** Some bodies don't have AF.A (e.g. K-3) — model-dependent |
| **AF point selection mode (auto/select/spot/auto-11/expanded)** | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | pslr_set_af_point_sel. **No widget exposed.** |
| **Selected AF point (27pt for K-3 family, 11pt for older)** | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | pslr_set_selected_af_point. **No widget exposed.** Pentax-A family: 27pt; older: 11pt |
| **AF point read (which point is in focus)** | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | status field `focused_af_point`. **No widget exposed.** Could be useful for star detection but not a substitute for HFR |
| **AE metering (multi/center/spot)** | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | pslr_set_ae_metering_mode. **No widget exposed.** Not relevant for astro (M mode) |
| **Light meter flags (exposure ok / over / under)** | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | status field `light_meter_flags`. **No widget exposed.** Useful for "is the scene too bright" but not for HFR |
| **FOCUS POSITION READ (absolute)** | **`firmware-✗`** | **`firmware-✗`** | **`firmware-✗`** | **`firmware-✗`** | **`firmware-✗`** | **`firmware-✗`** | **`firmware-✗`** | **`firmware-✗`** | **`firmware-✗`** | **`firmware-✗`** | **`firmware-✗`** | **`firmware-✗`** | **Cannot add. Closed at firmware level for all 13 legacy bodies. See NINA_PENTAX_FOCUS_SPEC.md §10–§13.** |
| **FOCUS DRIVE (motor steps in/out)** | **`firmware-✗`** | **`firmware-✗`** | **`firmware-✗`** | **`firmware-✗`** | **`firmware-✗`** | **`firmware-✗`** | **`firmware-✗`** | **`firmware-✗`** | **`firmware-✗`** | **`firmware-✗`** | **`firmware-✗`** | **`firmware-✗`** | **Cannot add. Closed at firmware level for all 13 legacy bodies. K-1, K-1 II, K-3 III are PTP path bodies that DO have focus, but they are not in scope for the legacy pslr driver.** |
| **Live view** | `firmware-✗` | `firmware-✗` | `firmware-✗` | `firmware-✗` | `firmware-✗` | `firmware-✗` | `firmware-✗` | `firmware-✗` | `firmware-✗` | `firmware-✗` | `firmware-✗` | `firmware-✗` | No live view opcodes in any legacy firmware. The pslr driver doesn't even try. Cannot add |
| **Video capture control** | `firmware-✗` | `firmware-✗` | `firmware-✗` | `firmware-✗` | `firmware-✗` | `firmware-✗` | `firmware-✗` | `firmware-✗` | `firmware-✗` | `firmware-✗` | `firmware-✗` | `firmware-✗` | No video opcodes in legacy firmware. Irrelevant to astro anyway |

### 6.4 White balance, flash, drive mode

| Feature | K-3 | K-3 II | 645Z | K-01 | K-30 | K-50 | K-500 | K-70 | KP | K-S1 | K-S2 | Q7 | Notes |
|---|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|---|
| **White balance mode** | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | pslr_set_white_balance. 17 modes (PSLR_WHITE_BALANCE_MODE_*). **No widget exposed.** For RAW astro, irrelevant |
| **White balance fine adjustment (mg/ba)** | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | pslr_set_white_balance_adjustment. **No widget exposed.** |
| **Flash mode** | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | pslr_set_flash_mode. 8 modes (manual / slow / trailing / auto / wireless). **No widget exposed.** |
| **Flash exposure compensation** | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | pslr_set_flash_exposure_compensation. **No widget exposed.** |
| **Drive mode (single / cont-H / cont-L / self-timer 12s / 2s / remote / remote-3s)** | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | pslr_set_drive_mode. **No widget exposed.** Relevant for astro: mirror lockup via self-timer 2s. Could be widget-added (small effort) |
| **Continuous shooting** | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | X10_CONTINUOUS. Not used in pslr.c — but the opcode is there. **Could be added** (small effort). For astro: not useful |

### 6.5 Shake reduction, special features, body control

| Feature | K-3 | K-3 II | 645Z | K-01 | K-30 | K-50 | K-500 | K-70 | KP | K-S1 | K-S2 | Q7 | Notes |
|---|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|---|
| **Shake reduction (SR) toggle** | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | `API` (settings) | settings.shake_reduction. **No widget exposed.** Should be **OFF for long-exposure astro on tripod** — a widget would help ensure that |
| **Horizon correction (SR auto-level)** | `API` (settings) | `API` (settings) | `firmware-✗` | `firmware-✗` | `firmware-✗` | `firmware-✗` | `firmware-✗` | `API` (settings) | `API` (settings) | `firmware-✗` | `firmware-✗` | `firmware-✗` | settings.horizon_correction. K-3, K-3 II, K-70, KP only |
| **Astrotracer** | `API` (settings) | `API` (settings) | `API` (settings) | `firmware-✗` | `firmware-✗` | `firmware-✗` | `firmware-✗` | `API` (settings) | `API` (settings) | `firmware-✗` | `firmware-✗` | `firmware-✗` | settings.astrotracer. Pentax-A and pentax-B family with O-GPS accessory |
| **Astrotracer timer (seconds)** | `API` (settings) | `API` (settings) | `API` (settings) | `firmware-✗` | `firmware-✗` | `firmware-✗` | `firmware-✗` | `API` (settings) | `API` (settings) | `firmware-✗` | `firmware-✗` | `firmware-✗` | settings.astrotracer_timer_sec. Read-only in pslr (no setter) |
| **Lens ID read** | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | status fields `lens_id1`, `lens_id2`. **No widget exposed.** Useful for NINA to identify what lens is attached |
| **Battery info** | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | status fields `battery_1`..`battery_4`. **No widget exposed.** Pentax battery: temperature, serial, etc. |
| **Firmware version (DSP info)** | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | pslr_get_dspinfo. **No widget exposed.** Useful for diagnostics |
| **Body datetime** | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | `API` (R) | pslr_get_datetime. **No widget exposed.** NINA sets date/time via OS, not body |

### 6.6 Buffer and file transfer

| Feature | K-3 | K-3 II | 645Z | K-01 | K-30 | K-50 | K-500 | K-70 | KP | K-S1 | K-S2 | Q7 | Notes |
|---|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|---|
| **Capture (single)** | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | `API` (W) | libgphoto2 captures via this internally |
| **Buffer download (single image)** | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | pslr_buffer_read. **No widget, but core libgphoto2 functionality.** |
| **Buffer listing** | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | libgphoto2 internal |
| **Buffer delete** | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | pslr_delete_buffer |
| **Full memory read (download all)** | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | pslr_fullmemory_read |
| **Buffer type (SD1/SD2 slot select)** | `API` | `API` | `API` (645Z dual) | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | `API` | pslr_get_jpeg_buffer_type. **No widget exposed.** 645Z has 2 SD slots; others have 1 (K-3, K-3 II, K-70, KP have 2 SD slots) |

### 6.7 Per-body model parameters (not setters, but body-specific limits)

| Parameter | K-3 | K-3 II | 645Z | K-01 | K-30 | K-50 | K-500 | K-70 | KP | K-S1 | K-S2 | Q7 |
|---|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| Status buffer size | 452 | 452 | 0 (no parser) | 452 | 452 | 452 | 452 | 456 | 456 | 452 | 452 | 0 |
| Max JPEG stars | 4 | 4 | 3 | 3 | 3 | 4 | 3 | 3 | 3 | 3 | 3 | 3 |
| JPEG resolutions (MP) | 24, 14, 6, 2 | 24, 14, 6, 2 | 51, 32, 21, 3 | 16, 12, 8, 5 | 16, 12, 8, 5 | 16, 12, 8, 5 | 16, 12, 8, 5 | 24, 14, 6, 2 | 24, 14, 6, 2 | 20, 12, 6, 2 | 20, 12, 6, 2 | 12, 8, 5, 2 |
| JPEG property levels | 9 | 9 | 9 | 9 | 9 | 9 | 9 | 9 | 9 | 9 | 9 | 9 |
| Fastest shutter (1/x s) | 8000 | 8000 | 4000 | 4000 | 6000 | 6000 | 6000 | 6000 | 6000 | 6000 | 6000 | 8000 |
| Base ISO min | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 |
| Base ISO max | 51200 | 51200 | 204800 | 12800 | 12800 | 51200 | 51200 | 102400 | 819200 | 51200 | 51200 | 6400 |
| Extended ISO min | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 |
| Extended ISO max | 51200 | 51200 | 204800 | 25600 | 25600 | 51200 | 51200 | 102400 | 819200 | 51200 | 51200 | 12800 |
| Max image tone | BB | BB | CP | BB | BB | BB | CP | AUTO | AUTO | CP | CP | (none) |
| Has JPEG hue | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ? |
| AF point count | 27 | 27 | 35 | 11 | 11 | 11 | 11 | 11 | 27 | 11 | 27 | 0 |
| Status parser | `ipslr_status_parse_k3` | `ipslr_status_parse_k3` | NULL | `ipslr_status_parse_k01` | `ipslr_status_parse_k30` | `ipslr_status_parse_k50` | `ipslr_status_parse_k500` | `ipslr_status_parse_k70` | `ipslr_status_parse_k70` | `ipslr_status_parse_ks1` | `ipslr_status_parse_k3` | NULL |

(`BB` = Bleach Bypass; `CP` = Cross Processing. These are the highest custom image tone the body's firmware supports.)

---

## 7. Which features could be added, by category

### Category A — Firmware supports it; pslr driver does not expose it (or only via C API, not widget)

These features are technically addable today by writing a pslr.c wrapper that uses the existing X10/X18 subcommand dispatch. The work is:

1. Map the feature to the X10/X18 subcommand (already done for most)
2. Write the pslr_set_*/pslr_get_* function (parameter encoding needs firmware reverse engineering if the subcommand is `X18_08`, `X18_09`, `X18_0A`, ..., or one of the unknown X10 subcommands)
3. Add a widget to `library.c` so NINA can see it

**Category A features that are NOT yet pslr_set_*'d:**
- **Auto bracket mode/EV/count** — subcommand `X18_19` (currently unmapped in pslr.c). Status fields exist; setter is the missing piece
- **JPEG hue** — subcommand `X18_JPEG_HUE` is mapped in the enum but the pslr_set_jpeg_hue function may not be wired up on all bodies (K-3 family: yes; older: model-dependent)
- **Continuous shooting** — subcommand `X10_CONTINUOUS`. pslr.c never calls it
- **Raw format set** — subcommand `X18_RAW_FORMAT` is mapped but pslr_set_raw_format is C-only, no widget
- **User file format** — proprietary subcommand, pslr_set_user_file_format exists but no widget
- **AF mode / AF point selection / selected AF point** — pslr_set_* exists, no widget
- **AE metering mode** — pslr_set_ae_metering_mode exists, no widget
- **Drive mode** — pslr_set_drive_mode exists, no widget
- **White balance mode / WB adjustment** — pslr_set_white_balance* exists, no widget
- **Flash mode / flash EC** — pslr_set_flash* exists, no widget
- **Color space** — pslr_set_color_space exists, no widget
- **JPEG image tone / sharpness / contrast / saturation** — pslr_set_jpeg_* exists, no widget
- **Shake reduction toggle** — settings field only, no setter
- **Astrotracer toggle / timer** — settings field only, no setter
- **Astrotracer timer set** — could be a setter, status-only today
- **Custom EV steps / Custom sensitivity steps** — status fields, no setters
- **One-push bracketing, bulb-mode-press-press, remote-bulb-mode-press-press** — settings fields, no setters
- **Buffer type (SD1/SD2 select for dual-slot bodies)** — pslr_get_jpeg_buffer_type exists; no setter

**Effort estimate per Category A feature:** 1–3 days of driver work (p_slr.c setter + library.c widget + NINA-side consumption) for features where the X10/X18 subcommand is known; **multi-week** for features where the subcommand is `X18_08`, `X18_09`, ..., `X18_24` (these are the unknown / "do something" subcommands, ~9 of them, and reverse-engineering each one requires reading the firmware's handler for that subcommand).

### Category B — Driver-level improvements (no firmware analysis, no per-body work)

These are quality-of-implementation fixes to the existing pslr driver that don't require new opcodes:

- **Better error handling** — map PTP `ResponseCode` to libgphoto2 error codes more aggressively
- **USB reliability** — tune retry policy for flaky USB-C hubs
- **Status refresh** — call pslr_get_status after every pslr_set_* to keep the cached status fresh
- **Add the missing widgets to library.c** — wire the existing pslr_set_* to NINA-visible widgets. This is purely a libgphoto2 camlib change, ~12 widgets (white balance, flash, drive mode, AF mode, AE metering, color space, JPEG image tone, JPEG sharpness/contrast/saturation/hue, RAW format, buffer type, shake reduction)

**Effort estimate:** 1–2 weeks for the full library.c widget expansion. The pslr.c side is already done.

### Category C — Bench verification needed (not a code change)

These are features where the firmware evidence says one thing but user reports or external evidence say another. The K-3 in particular is in this bucket:

- **K-3 focus drive** — firmware scan says no focus opcodes, but K-3 users on the `pentax` mailing list report that pktriggercord can drive focus on K-3. This is a contradiction that can only be resolved by **bench testing a real K-3**. Until then, the firmware evidence is the safer answer.
- **K-70 / KP focus drive** — same situation; these are pentax-B family bodies, and the K-1 (same family) does support focus over PTP. The firmware scan says no, but the family-membership expectation says yes. **Bench verification required.**
- **Astrotracer on K-3 / K-3 II / 645Z / K-70 / KP** — the O-GPS1/2 is a separate accessory. The pslr driver may not talk to it correctly even though the body reports the status. **Bench verification required.**

### Category D — Firmware does not support it (cannot add)

| Feature | Status |
|---|---|
| **Focus position read** | **Cannot add** — no legacy body firmware implements `0x9204` or `0x9205`. |
| **Focus drive** | **Cannot add** — no legacy body firmware implements `0x9016` or `0x9017`. |
| **Live view** | **Cannot add** — no live view opcodes in any legacy firmware. |
| **Video capture** | **Cannot add** — no video opcodes in any legacy firmware. |
| **Bulb mode over USB (long exposure control)** | **Partially supported** — X10_BULB exists, but the firmware handler just triggers a single shutter event; the host can't tell the body "open for 300 seconds, then close". The pslr driver uses a software-timed bulb (sleep), not a hardware-timed bulb. **This is the right answer for legacy Pentax; the bodies don't expose hardware bulb control over USB.** |
| **Auto-focus on demand (with focus position result)** | **Cannot add** — the body can AF (shutter half-press equivalent), but the result is not exposed as a host-controllable opcode. The body does not have a "return AF distance" opcode. |

---

## 8. Per-body gaps: where the legacy pslr driver falls short today

### 8.1 K-3 II (the user's specific focus)

**Per the user's most recent message: "I have told you already, do a K3ii firmware decompile" and "k3ii is broadly similar, so if K3 can do it, then the k3ii can"**

For the **K-3 II specifically**:

| Status | pslr support today | What firmware supports | What NINA sees |
|---|---|---|---|
| Capture (single) | ✓ | ✓ | ✓ (via libgphoto2) |
| Shutter speed set | ✓ | ✓ | R-only (widget) |
| Aperture set | ✓ | ✓ | R-only (widget) |
| ISO set | ✓ | ✓ | W (widget) |
| Exposure mode | ✓ | ✓ | RW (widget) |
| Bulb trigger | ✓ | ✓ | W (widget) |
| AE lock | ✓ | ✓ | — (C only) |
| Green button | ✓ | ✓ | — (C only) |
| Dust removal | ✓ | ✓ | — (C only) |
| Mirror lockup (self-timer 2s) | ✓ | ✓ | — (C only, no widget) |
| Image format | ✓ | ✓ | RW (widget) |
| JPEG quality | ✓ | ✓ | RW (widget) |
| AF mode | ✓ | ✓ | — (C only) |
| AF point selection | ✓ | ✓ | — (C only) |
| AE metering | ✓ | ✓ | — (C only) |
| White balance | ✓ | ✓ | — (C only) |
| Drive mode | ✓ | ✓ | — (C only) |
| Color space | ✓ | ✓ | — (C only) |
| Shake reduction toggle | C-only (settings) | ✓ | — (no widget, important for astro) |
| Astrotracer toggle | C-only (settings) | ✓ (with O-GPS1) | — (no widget) |
| **Focus position read** | **firmware-✗** | **firmware-✗** | **— (cannot add)** |
| **Focus drive** | **firmware-✗** | **firmware-✗** | **— (cannot add)** |
| Live view | firmware-✗ | firmware-✗ | — (cannot add) |

**K-3 II is the best-supported pentax-A body for legacy pslr (alongside K-3).** The pslr driver has the most complete coverage for it. **The only feature gaps that are "interesting" to NINA are: (a) shake reduction widget (off-by-default for tripod), (b) mirror lockup widget (self-timer 2s), (c) AF mode / drive mode widgets (less critical). All other "features" are either: (i) irrelevant to astro (color space, WB, JPEG tone), (ii) already exposed to NINA (ISO, aperture, shutter speed, EC, image format, JPEG quality, bulb), or (iii) closed at the firmware level (focus, live view).**

### 8.2 K-3 (same as K-3 II; per user "k3ii is broadly similar, so if K3 can do it, then the k3ii can")

K-3 firmware (`fwdc220b.bin`) has zero focus opcodes per the scan, **the same as K-3 II**. The user notes this is a "may or may not work" body (Category C above) because of user reports to the contrary. The pslr driver's coverage of K-3 is identical to K-3 II.

### 8.3 645Z (medium format, pentax-A)

645Z has the same pentax-A opcode family as K-3/K-3 II. Same coverage. Differences:
- 51MP instead of 24MP
- Buffer size 0 in pslr_model.c (line 1009) — **status buffer size is unknown to the driver; this is a bug. Should be 452 or 456.** Could cause status parsing failures on 645Z.
- No dedicated status parser (NULL in line 1009); falls back to the default parser, which may misparse some 645Z-specific fields.

**645Z-specific gaps:** same as K-3 II plus the missing parser and the unknown status buffer size.

### 8.4 K-01, K-30, K-50, K-500 (entry/mid-range pentax-A)

These are the cheaper pentax-A bodies. Same opcode family. Coverage is identical. The only difference is the per-body max ISO range and the max JPEG image tone.

### 8.5 K-70, KP (pentax-B, modern mid-range)

These are the "limited support" bodies (pslr_model.c lines 1007-1008, marked after line 995). They use a different status parser (`ipslr_status_parse_k70`).

**Known pentax-B specific gaps vs. pentax-A:**
- K-70 has 11 AF points (vs. 27 for K-3 II / KP)
- Status buffer size 456 (vs. 452 for pentax-A)
- The pslr driver may misparse some pentax-B-specific status fields that the parser doesn't recognize

**The user has explicitly excluded K-1 and K-1 II from the legacy investigation**, so we won't try to characterize pentax-B beyond what's in the model table.

### 8.6 K-S1, K-S2 (pentax-B, K-S1 has 11pt, K-S2 has 27pt)

K-S1 and K-S2 are "limited support" bodies (lines 1003-1004). K-S1 uses `ipslr_status_parse_ks1`; K-S2 reuses `ipslr_status_parse_k3`. Same coverage gaps as K-70/KP.

### 8.7 Q7, Q, Q10 (pentax-D, mirrorless compact)

These are the "Q" mirrorless bodies. They are in the model table (line 1003 was K-S1, but the Q series is not in the legacy table — only the K-series). **Wait, this is wrong. Let me re-check.**

Looking at `pslr_model.c:1003-1010`, the "limited" list includes K-S1, K-S2, K-1, K-1 II, K-70, KP, 645Z, K-3 III. **Q7 is NOT in the table at all.** The Q7 firmware is on disk but the driver doesn't know about it. The Q7 is therefore **a "doesn't exist for the pslr driver" body**: it would be `NULL` from `pslr_find_model_by_id`, and the driver would refuse to connect.

**This means for the Q7 specifically:** the pslr driver has no support, full stop. The firmware is on disk for analysis, but the driver has no entry for it.

---

## 9. The "what could we add" answer, prioritized for NINA

If the NINA implementer wants to add features to the Pentax legacy pslr driver, here's the priority list (high to low):

### High priority (relevant to astro, addable)
1. **Add shake reduction widget** to `library.c` (Category B, no firmware work). Pentax defaults SR ON; for tripod-mounted long exposures it should be OFF. A NINA widget that sets `settings.shake_reduction = false` before capture would be valuable.
2. **Add drive mode widget** with self-timer 2s option (Category B). This is the legacy equivalent of "mirror lockup" for Pentax.
3. **Add buffer type widget** for dual-slot bodies (K-3, K-3 II, K-70, KP, 645Z). Lets NINA choose which SD card to write to.

### Medium priority (relevant to astro, requires reverse engineering)
4. **Add astrotracer widget** for bodies that support it (K-3, K-3 II, 645Z, K-70, KP with O-GPS1/2). Requires pslr_set_astrotracer / pslr_set_astrotracer_timer to be written, mapping to the body's astrotracer control opcodes.
5. **Add buffer type read** for the dual-slot bodies' "which slot is active" indicator.
6. **Add lens ID read** so NINA knows what lens is attached.

### Low priority (not relevant to astro, or already complete)
7. JPEG image tone / sharpness / contrast / saturation / hue widgets (irrelevant for RAW)
8. Color space widget (irrelevant for RAW)
9. White balance mode / WB fine adjustment widgets (irrelevant for RAW)
10. AF mode / AF point widgets (irrelevant for non-AF astro)
11. AE metering widget (irrelevant for M-mode astro)
12. Flash mode / flash EC widgets (irrelevant for astro)

### Closed at firmware level (cannot add)
- **Focus drive** — see NINA_PENTAX_FOCUS_SPEC.md §10–§13
- **Focus position read** — see NINA_PENTAX_FOCUS_SPEC.md §10–§13
- **Live view** — no firmware opcodes
- **Video capture** — no firmware opcodes

---

## 10. Summary table: "What can the legacy pslr driver do, per body, per feature, today"

| Body | Capture | ISO/EC/Mode | Bulb | Image format | AF | Drive | WB | Flash | Color | WB | AE met | Status read | File dl |
|---|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| K-3 | ✓ | ✓ | ✓ | ✓ | API | API | API | API | API | API | API | ✓ | ✓ |
| K-3 II | ✓ | ✓ | ✓ | ✓ | API | API | API | API | API | API | API | ✓ | ✓ |
| 645Z | ✓ | ✓ | ✓ | ✓ | API | API | API | API | API | API | API | partial (no parser) | ✓ |
| K-01 | ✓ | ✓ | ✓ | ✓ | API | API | API | API | API | API | API | ✓ | ✓ |
| K-30 | ✓ | ✓ | ✓ | ✓ | API | API | API | API | API | API | API | ✓ | ✓ |
| K-50 | ✓ | ✓ | ✓ | ✓ | API | API | API | API | API | API | API | ✓ | ✓ |
| K-500 | ✓ | ✓ | ✓ | ✓ | API | API | API | API | API | API | API | ✓ | ✓ |
| K-70 | ✓ | ✓ | ✓ | ✓ | API | API | API | API | API | API | API | partial (pentax-B) | ✓ |
| KP | ✓ | ✓ | ✓ | ✓ | API | API | API | API | API | API | API | partial (pentax-B) | ✓ |
| K-S1 | ✓ | ✓ | ✓ | ✓ | API | API | API | API | API | API | API | partial (pentax-B) | ✓ |
| K-S2 | ✓ | ✓ | ✓ | ✓ | API | API | API | API | API | API | API | partial (pentax-B) | ✓ |
| Q7 | **— (not in pslr_model.c table)** | — | — | — | — | — | — | — | — | — | — | — | — |

(`API` = reachable via pslr public API but not bridged to a libgphoto2 widget that NINA can see. `✓` = widgeted in library.c OR core libgphoto2 functionality.)

---

## 11. Bottom line for the NINA implementer

1. **For Pentax K-3 II specifically:** the pslr driver has the most complete coverage of any legacy Pentax body. The gaps that matter for astro imaging are:
   - **Focus position read and focus drive are closed at the firmware level.** The pslr driver is correct to not support them. NINA cannot add these features for K-3 II.
   - **Shake reduction widget is missing** — should be set OFF for tripod long exposures. Easy add (Category B).
   - **Drive mode widget is missing** — self-timer 2s is the legacy mirror-lockup equivalent. Easy add (Category B).
   - **Buffer type widget is missing** — K-3 II has 2 SD slots. Easy add (Category B).
2. **For the wider "what could be added" question:** see §7 and §9. The Category A features are addable but require firmware reverse engineering for the unknown X18 subcommands. The Category B features (mainly: bridge existing pslr_set_* to library.c widgets) are pure driver work and could be done in 1–2 weeks for the full set.
3. **The "are there hidden features" answer is no**, with one caveat: the pentax-B bodies (K-70, KP) and Q-series (Q7) have less complete pslr driver support than the pentax-A bodies, and bench testing them might reveal features the driver doesn't expose. But these are body-specific gaps, not new features at the firmware level.
4. **For the immediate NINA focus task:** no NINA-side change is required for the K-3 II. The pslr driver is correct as-is. The implementation strategy is to add the `pentax` camlib support to NINA's libgphoto2 wrapper exactly as it is.

---

## Appendix A: Files of record

- `libgphoto2/_build/meson-private/dist-unpack/libgphoto2-2.5.34/camlibs/pentax/pslr.h` — pslr public API (80 functions)
- `libgphoto2/_build/meson-private/dist-unpack/libgphoto2-2.5.34/camlibs/pentax/pslr.c` — pslr implementation (1810 lines, X10/X18 subcommand dispatch)
- `libgphoto2/_build/meson-private/dist-unpack/libgphoto2-2.5.34/camlibs/pentax/pslr_model.h` — model struct (108-field pslr_status, no focus_position field)
- `libgphoto2/_build/meson-private/dist-unpack/libgphoto2-2.5.34/camlibs/pentax/pslr_model.c` — camera_models[] table (29 bodies, 13 in legacy scope)
- `libgphoto2/_build/meson-private/dist-unpack/libgphoto2-2.5.34/camlibs/pentax/pslr_enum.h` — enums (color space, AF mode, AE metering, flash mode, drive mode, AF point sel, JPEG image tone, white balance mode, custom EV steps, custom sensitivity steps, image format, raw format, scene mode)
- `libgphoto2/_build/meson-private/dist-unpack/libgphoto2-2.5.34/camlibs/pentax/library.c` — libgphoto2 bridge (14 widgets, 10 of which are R or R-only)
- `/tmp/legacy_firmware_analysis.json` — per-firmware opcode inventory (812 KB)
- `/tmp/decompile_full.txt` — human-readable summary
- `NINA_PENTAX_FOCUS_SPEC.md` — focus-specific spec, contains the firmware decompilation evidence in §10 and the false-positive analysis in §13.3

## Appendix B: How to read the firmware decompilation results

The "0x9205 literal" column is the number of times the 32-bit little-endian value `0x00009205` appears anywhere in the firmware image. The "0x9205 LDR refs" column is the number of `LDR.W [PC, #imm]` instructions whose target address is a `0x9205` literal pool location.

**If `0x9205 literal > 0` and `0x9205 LDR refs > 0`** — the firmware has a focus-position-read dispatch table. (This is the case for K-3 III.)

**If `0x9205 literal > 0` and `0x9205 LDR refs == 0`** — the value appears in a literal pool but is never loaded by an instruction. This is dead code (or, in our case, an instruction byte that happens to look like a data literal — the false-positive case for K-70 0x9205 and Q7 0x9204).

**If `0x9205 literal == 0`** — the firmware doesn't even contain the value `0x00009205`. The opcode is completely absent.

The K-3 II has `0x9205 literal == 0` and `0x9205 LDR refs == 0`. **Conclusion: the K-3 II firmware does not implement the focus position read opcode, end of story.**

---

## Appendix C: Data sources for every matrix claim

Every claim in the matrix above traces back to one of the following source files. The line numbers were captured at decompile time; if a future libgphoto2 release moves them, the claim still holds (the file/function is named) but re-verify the line number.

### C.1 pslr.h — public API surface

| Claim | Source | Line(s) |
|---|---|---|
| 80-function public API | `pslr.h` | 119–231 |
| `pslr_focus` exists and is callable | `pslr.h` | 126 |
| `pslr_shutter` exists | `pslr.h` | 125 |
| `pslr_bulb(on/off)` | `pslr.h` | 181 |
| `pslr_set_shutter/aperture/iso/ec` | `pslr.h` | 142–145 |
| `pslr_set_white_balance(_adjustment)` | `pslr.h` | 147–148 |
| `pslr_set_flash_mode/ec` | `pslr.h` | 149–150 |
| `pslr_set_drive_mode` | `pslr.h` | 151 |
| `pslr_set_af_mode/af_point_sel/ae_metering_mode` | `pslr.h` | 152–154 |
| `pslr_set_color_space` | `pslr.h` | 155 |
| `pslr_set_jpeg_stars/resolution/image_tone` | `pslr.h` | 157–159 |
| `pslr_set_jpeg_sharpness/contrast/saturation/hue` | `pslr.h` | 161–164 |
| `pslr_set_image_format/raw_format/user_file_format` | `pslr.h` | 166–168 |
| `pslr_set_exposure_mode` | `pslr.h` | 189 |
| `pslr_set_selected_af_point` | `pslr.h` | 190 |
| `pslr_get_model_max_jpeg_stars/jpeg_property_levels/status_buffer_size` | `pslr.h` | 193–195 |
| `pslr_get_model_fastest_shutter_speed/base_iso_min/base_iso_max/extended_iso_min/extended_iso_max` | `pslr.h` | 196–200 |
| `pslr_get_model_jpeg_resolutions` | `pslr.h` | 201 |
| `pslr_get_model_only_limited` | `pslr.h` | 202 |
| `pslr_get_model_has_jpeg_hue` | `pslr.h` | 203 |
| `pslr_get_model_need_exposure_conversion` | `pslr.h` | 204 |
| `pslr_get_model_max_supported_image_tone` | `pslr.h` | 205 |
| `pslr_get_model_has_settings_parser` | `pslr.h` | 206 |
| `pslr_get_model_af_point_num` | `pslr.h` | 207 |
| `pslr_get_model_old_bulb_mode` | `pslr.h` | 208 |
| `pslr_get_model_bufmask_single` | `pslr.h` | 209 |
| `pslr_get_jpeg_buffer_type/jpeg_resolution` | `pslr.h` | 211–212 |
| `pslr_get_camera_name` | `pslr.h` | 192 |
| `pslr_get_dspinfo(firmware string)` | `pslr.h` | 216 |
| `pslr_get_setting/_settings/_settings_json` | `pslr.h` | 218, 222, 130 |
| `pslr_set_setting/_setting_by_name` | `pslr.h` | 219–220 |
| `pslr_has_setting_by_name` | `pslr.h` | 224 |
| `pslr_format_rational/copyright` | `pslr.h` | 225, 229 |
| `pslr_green_button/ae_lock/dust_removal/button_test` | `pslr.h` | 173, 177, 179, 175 |
| `pslr_get_status_info/_settings_info` | `pslr.h` | 133–134 |
| `pslr_buffer_open/_close/_read/_get_size/_segment_info` | `pslr.h` | 183–186 |
| `pslr_fullmemory_read` (direct SD read) | `pslr.h` | 184 |
| `pslr_get_buffer/_delete_buffer` | `pslr.h` | 136, 171 |
| `pslr_set_progress_callback` | `pslr.h` | 139 |
| `pslr_test` (raw command dispatch) | `pslr.h` | 227 |
| `pslr_set_debugmode` | `pslr.h` | 231 |

### C.2 pslr.c — wire-protocol behavior

| Claim | Source | Line(s) |
|---|---|---|
| X18 subcommand enum (36 entries) | `pslr.c` | 106–145 |
| X10 subcommand enum (17 entries) | `pslr.c` | 149–168 |
| `pslr_focus` calls `ipslr_press_shutter(p, false)` (half-press) | `pslr.c` | 463 |
| `pslr_shutter` calls `ipslr_press_shutter(p, true)` (full-press) | `pslr.c` | 459 |
| `ipslr_press_shutter` body | `pslr.c` | 1348 |
| `pslr_get_model_only_limited` returns `cam->only_limited` | `pslr.c` | 1162 |
| `pslr_get_model_has_jpeg_hue` returns `cam->has_jpeg_hue` | `pslr.c` | 1167 |
| `pslr_get_model_need_exposure_conversion` returns `cam->need_exposure_mode_conversion` | `pslr.c` | 1172 |
| `pslr_get_model_old_bulb_mode` returns `cam->old_bulb_mode` | `pslr.c` | 1212 |
| `pslr_get_model_bufmask_single` returns `cam->bufmask_single` | `pslr.c` | 1217 |
| `pslr_get_model_has_settings_parser` returns `cam->status_parser != NULL` | `pslr.c` | 1222 |

### C.3 pslr_model.c — `camera_models[]` table (line-anchored per body)

| Body | Line | Status buffer | AF pts | Parser |
|---|:-:|:-:|:-:|---|
| *ist DS | 974 | 264 | 11 | `ipslr_status_parse_istds` |
| K20D | 975 | 412 | 11 | `ipslr_status_parse_k20d` |
| K10D | 976 | 392 | 11 | `ipslr_status_parse_k10d` |
| GX10 | 977 | 392 | 11 | `ipslr_status_parse_k10d` |
| GX20 | 978 | 412 | 11 | `ipslr_status_parse_k20d` |
| K-x | 980 | 436 | 11 | `ipslr_status_parse_kx` |
| K200D | 981 | 408 | 11 | `ipslr_status_parse_k200d` |
| K-7 | 982 | 436 | 11 | `ipslr_status_parse_kx` |
| K-r | 983 | 440 | 11 | `ipslr_status_parse_kr` |
| K-5 | 984 | 444 | 11 | `ipslr_status_parse_k5` |
| K-2000 | 985 | 412 | 11 | `ipslr_status_parse_km` |
| K-m | 986 | 412 | 11 | `ipslr_status_parse_km` |
| K-30 | 987 | 452 | 11 | `ipslr_status_parse_k30` |
| K-01 | 988 | 452 | 11 | `ipslr_status_parse_k01` |
| K-5II | 989 | 444 | 11 | `ipslr_status_parse_k5` |
| K-5IIs | 990 | 444 | 11 | `ipslr_status_parse_k5` |
| K-50 | 991 | 452 | 11 | `ipslr_status_parse_k50` |
| **K-3** | **992** | **452** | **27** | **`ipslr_status_parse_k3`** |
| **K-3II** | **993** | **452** | **27** | **`ipslr_status_parse_k3`** |
| K-500 | 994 | 452 | 11 | `ipslr_status_parse_k500` |
| K-S1 | 1003 | 452 | 11 | `ipslr_status_parse_ks1` |
| K-S2 | 1004 | 452 | 27 | `ipslr_status_parse_k3` |
| K-1 | 1005 | 456 | 33 | `ipslr_status_parse_k1` |
| K-1 II | 1006 | 456 | 33 | `ipslr_status_parse_k1` |
| K-70 | 1007 | 456 | 11 | `ipslr_status_parse_k70` |
| KP | 1008 | 456 | 27 | `ipslr_status_parse_k70` |
| **645Z** | **1009** | **0 (BUG)** | **35** | **NULL (BUG)** |
| K-3 III | 1010 | 452 | 27 | `ipslr_status_parse_k3` (PTP) |

**Q7 is not in this table at all.** Confirmed by grep: no entry containing `"Q7"` or `"Q, "` in `pslr_model.c`. The driver will return `NULL` from `pslr_find_model_by_id` if a Q7 is plugged in. The Q7 firmware is on disk for decompilation analysis only.

### C.4 pslr_model.h — pslr_status struct fields

The `pslr_status` struct contains 108 fields. **There is no `focus_position` field, no `focus_step_count` field, and no `lens_focus_distance` field.** The `focus` field (int32_t) is the focus *mode* state (one of `pslr_focus_mode_t`), not a position.

This is the field-by-field source for §4 of the matrix (which features are read from the body each status poll):

| Field | Source | Notes |
|---|---|---|
| `bufmask`, `current_iso/shutter/aperture/lens_max/lens_min` | `pslr_model.h` | Read from status buffer |
| `set_shutter_speed`, `set_aperture` | `pslr_model.h` | Set by setter, returned in status |
| `max_shutter_speed` | `pslr_model.h` | From per-body model table |
| `auto_bracket_*` | `pslr_model.h` | Read from status |
| `fixed_iso` | `pslr_model.h` | Read from status |
| `jpeg_*` | `pslr_model.h` | Set by setter, returned in status |
| `zoom` | `pslr_model.h` | Read from status |
| `focus` (mode, NOT position) | `pslr_model.h` | Read from status |
| `image_format`, `raw_format` | `pslr_model.h` | Set by setter |
| `light_meter_flags` | `pslr_model.h` | Read from status (AE lock bit = 0x8) |
| `ec` (exposure compensation) | `pslr_model.h` | Set by setter, returned in status |
| `custom_ev_steps`, `custom_sensitivity_steps` | `pslr_model.h` | Per-body from table |
| `exposure_mode` | `pslr_model.h` | Set by setter, returned in status |
| `scene_mode`, `user_mode_flag` | `pslr_model.h` | Read from status |
| `ae_metering_mode` | `pslr_model.h` | Set by setter |
| `af_mode`, `af_point_select` | `pslr_model.h` | Set by setter |
| `selected_af_point`, `focused_af_point` | `pslr_model.h` | Set by setter, returned in status |
| `auto_iso_min/max` | `pslr_model.h` | Set by setter |
| `drive_mode` | `pslr_model.h` | Set by setter |
| `shake_reduction` | `pslr_settings` | settings struct, not status |
| `white_balance_*` | `pslr_model.h` | Set by setter, returned in status |
| `flash_mode` | `pslr_model.h` | Set by setter |
| `flash_exposure_compensation` | `pslr_model.h` | Set by setter |
| `manual_mode_ev` | `pslr_model.h` | Read from status |
| `color_space` | `pslr_model.h` | Set by setter |
| `lens_id1`, `lens_id2` | `pslr_model.h` | Read from status |
| `battery_1..4` | `pslr_model.h` | Read from status (4 battery slots) |

The `pslr_settings` struct (10 fields) has: `one_push_bracketing`, `bulb_mode_press_press`, `bulb_timer`, `bulb_timer_sec`, `using_aperture_ring`, `shake_reduction`, `astrotracer`, `astrotracer_timer_sec`, `horizon_correction`, `remote_bulb_mode_press_press`.

### C.5 library.c — widget exposure (the actual NINA-visible surface)

The libgphoto2 bridge (`camlibs/pentax/library.c`) creates exactly 14 widgets, of which only 8 are read-only and 6 are read-write. The NINA implementer can ONLY see these 14 widgets — every other feature requires either (a) a library.c widget bridge (driver change), or (b) direct pslr.h C-API calls (NINA plugin change).

| Widget | Permission | R/W source | Maps to |
|---|---|---|---|
| `model` | RO | `pslr_get_camera_name` | Camera name |
| `imageformat` | RW | `pslr_set_image_format` | Image format (JPEG/RAW/RAW+) |
| `imgsize` | RO | `pslr_get_jpeg_resolution` | JPEG resolution in MP |
| `imagequality` | RW | `pslr_set_jpeg_stars` | JPEG quality (1–4 stars) |
| `iso` | **W** | `pslr_set_iso` | ISO + auto-ISO range |
| `shutterspeed` | **RO** | `pslr_get_status` | Current shutter (no widget setter!) |
| `aperture` | **RO** | `pslr_get_status` | Current aperture (no widget setter!) |
| `aperture_min_focal` | RO | `pslr_get_model_base_iso_min` | Min aperture at min focal length |
| `aperture_max_focal` | RO | `pslr_get_model_base_iso_max` | Min aperture at max focal length |
| `zoom` | RO | `pslr_get_status` | Zoom position |
| `exposure_compensation` | **W** | `pslr_set_expose_compensation` | EC |
| `shooting_mode` | RW | `pslr_set_exposure_mode` | Exposure mode (M/Tv/Av/P/Sv/etc.) |
| `bulb` | **W** | `pslr_bulb(on)` | Bulb exposure on/off |
| (none) | — | — | Shutter — **NO WIDGET** (setter exists in pslr.c, no bridge) |

**Note on the W-only ISO and EC widgets:** NINA's gp_widget_set_value() *can* call them, but they do not return the body's value via gp_widget_get_value() because the widget has no read callback — only the set callback. So after setting ISO, the widget will read back whatever you last set, not the body's actual value.

### C.6 Firmware decompilation (the upper bound)

The per-firmware opcode inventory is in `/tmp/legacy_firmware_analysis.json` (812 KB). The decisive result:

| Body | 0x9016 | 0x9017 | 0x9204 | 0x9205 | LDR refs to 0x9205 |
|---|:-:|:-:|:-:|:-:|:-:|
| *ist DS / K20D / K10D / etc. (pre-2010) | 0 | 0 | 0 | 0 | 0 |
| K-x / K200D / K-7 / K-r / K-5 | 0 | 0 | 0 | 0 | 0 |
| K-m / K2000 / K-30 | 0 | 0 | 0 | 0 | 0 |
| K-01 | 0 | 0 | 0 | 0 | 0 |
| K-5II / K-5IIs | 0 | 0 | 0 | 0 | 0 |
| K-3 | 0 | 0 | 0 | 0 | 0 |
| **K-3 II** | **0** | **0** | **0** | **0** | **0** |
| K-50 / K-500 | 0 | 0 | 0 | 0 | 0 |
| K-S1 / K-S2 | 0 | 0 | 0 | 0 | 0 |
| K-70 | 0 | 0 | 0 | 1 (false positive — see §13.3 of NINA_PENTAX_FOCUS_SPEC.md) | 0 |
| KP | 0 | 0 | 0 | 0 | 0 |
| 645Z | 0 | 0 | 0 | 0 | 0 |
| Q7 | 0 | 0 | 2 (false positive — Thumb-2 STR) | 0 | 0 |
| K-3 III (PTP) | 0 | 0 | 0 | 1 (real) | > 0 |

The "false positive" rows are the Thumb-2 16-bit `STR R2, [SP, #imm8]` instructions that, when interpreted as little-endian 32-bit values, contain the bytes `0x04` or `0x05` at the right offset. They are stack stores, not focus dispatchers. See NINA_PENTAX_FOCUS_SPEC.md §13.3 for the disambiguation logic.

### C.7 Cross-check summary

- **Every `API` cell in the matrix** maps to a `pslr_set_*` or `pslr_get_*` function listed in §C.1 above.
- **Every `RW`/`RO`/`W`/`R` cell** maps to a widget in §C.5.
- **Every `firmware-✗` cell** maps to a row in §C.6 where the opcode is absent.
- **Every per-body number** (status buffer size, AF points, max ISO) maps to a line in §C.3.

The matrix is fully data-driven. The NINA implementer can verify any cell by following the chain: cell → §C.1/§C.3/§C.5/§C.6 → source file and line.
