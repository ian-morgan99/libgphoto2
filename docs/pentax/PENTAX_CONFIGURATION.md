# Pentax configuration integration

Revision: 2026-08-21

Pentax configuration must remain descriptor-driven. Vendor mode does not grant
permission to invent datatypes or enumerations, and the absence of a widget is
safer than writing a guessed value.

## Existing generic coverage

The ptp2 configuration engine already creates the following stable widgets when
the camera advertises the corresponding property and its descriptor has the
expected datatype:

| Widget | PTP property | Expected type in current handler | Status |
|---|---:|---:|---|
| `iso` | ExposureIndex `0x500f` | UINT16 | Absent on K-3 III firmware 2.20; do not expose through this path |
| `f-number` / `aperture` | FNumber `0x5007` | UINT16 | Hardware descriptor verified read/write |
| `whitebalance` | WhiteBalance `0x5005` | UINT16 | Hardware descriptor verified read/write |
| `focusmode` | FocusMode `0x500a` | UINT16 | Absent on K-3 III firmware 2.20; do not expose through this path |
| `exposurecompensation` | ExposureBiasCompensation `0x5010` | INT16 | Hardware descriptor verified read/write |
| `shutterspeed` | Pentax/Ricoh `0xd00f` | UINT64 rational | K-3 III descriptor read; K-1 II direct read verified, but an acknowledged M-mode write was ignored, so general write support is not yet proven |

The `0xd00f` constant already existed as `PTP_DPC_RICOH_ShutterSpeed`; the new
Pentax name is an alias for the same wire property. K-1 II does not advertise
it in DeviceInfo, so the exact-model `pentaxdirectshutter` widget requests its
descriptor on demand. Its setter verifies the post-write descriptor and fails
if a camera acknowledges but retains the old value.

On K-1 II in physical M, 1/500→1/125 produced the exact IT2 payload and PTP OK,
but immediate and fresh-session reads retained 1/500. The explicit restoration
write and independent verification also read 1/500. No preview followed. Audit
the conditions changeability word at offset 504 before another shutter write;
GetSet and physical M are not sufficient authorization or success evidence.

Read-only widget decoding on firmware 2.20 passed for white balance, aperture,
exposure compensation, and shutter speed. Several white-balance enumeration
values remain deliberately labelled `Unknown value` because the existing
Pentax table does not name them; no labels are guessed.

A controlled cold-start sequence subsequently reconfirmed shutter property
`0xd00f` through read-only `GetDevicePropDesc` (`0x1014`): UINT64 rational,
current 1 second, with 55 choices ranging from 1/8000 through 30 seconds. No
property value was written. The identical result passed in three controlled
power cycles after explicit shooting-screen readiness; the larger lifecycle
repetition gate remains open.

Three immediately subsequent warm reconnect sessions returned the same current
value and descriptor. Each independently enabled and disabled vendor mode, and
no session retained vendor state into the next.

## Deliberately withheld widgets

- Extended ISO `0xd01e` (UINT32; 3200→1600 was confirmed by PTP read-back and
  the camera display, but the battery failed before restoration to 3200, so no
  general setter is exposed until that restoration and more bounded values pass)
- Focus mode (no `0x500a` descriptor; no replacement property yet proven)
- Drive mode `0xd013`
- Exposure-bracketing mode/step `0xd014`/`0xd015`
- Color temperature `0xd018`
- Writing-file-format structure `0xd01b`
- Live-view zoom/mode `0xd036`/`0xd037`

The Windows client establishes that these codes are used. The H1.2 read-only
probe establishes several real types and GetSet flags, but not enough to prove
their numeric semantics, mode constraints, or compound payload layout. They
remain unexposed until one-property round-trip tests are explicitly approved.

## Read-only aggregate status

`status/pentaxconditions` performs one `GetAllConditions` (`0x900f`) read and
has no setter. It reports activity/changeability flags, ISO, Bulb timer state
and rational value, raw exposure/user/drive modes, and source-traced Astro
phase/error/limit fields. The parser requires 508 bytes before reading the
mandatory final field and separately requires 532 bytes for the optional Astro
limit. Unknown modes remain numeric. This widget must not be interpreted as an
Astro-ready decision or an exposure command.

## Hardware enactment checklist

Test physical Bulb (`B`) as a required mode separate from Manual: compare
shutter representation, bulb timer/long-exposure controls, Astrotracer-related
properties, and descriptor writability without triggering an exposure. Saved
U1–U3 profiles may be tested later, but first record their existing assignments;
changing a U profile is persistent and must have an explicit restoration plan.

For each target body and firmware:

1. Save `--list-all-config` and raw property descriptors before changing state.
2. Confirm property code, datatype, GetSet, form flag, current/default value,
   and every enum/range member.
3. Read the widget through gphoto2 and compare with the camera display.
4. Set one supported non-current value, refetch the descriptor/widget, and
   compare with the camera display.
5. Restore the exact original value and refetch again.
6. Repeat in relevant exposure, live-view, still/movie, and card modes.
7. Record unsupported/read-only modes rather than overriding GetSet.

Only after those records exist should a missing `submenu` entry or typed handler
be added, one property per commit as required by the canonical plan.
