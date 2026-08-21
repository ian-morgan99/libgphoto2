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
| `shutterspeed` | Pentax/Ricoh `0xd00f` | UINT64 rational | Hardware descriptor verified read/write; Pentax model-routing required because DeviceInfo vendor is Microsoft MTP |

The `0xd00f` constant already existed as `PTP_DPC_RICOH_ShutterSpeed`; the new
Pentax name is an alias for the same wire property. Do not add a duplicate menu
entry.

Read-only widget decoding on firmware 2.20 passed for white balance, aperture,
exposure compensation, and shutter speed. Several white-balance enumeration
values remain deliberately labelled `Unknown value` because the existing
Pentax table does not name them; no labels are guessed.

A controlled cold-start sequence subsequently reconfirmed shutter property
`0xd00f` through read-only `GetDevicePropDesc` (`0x1014`): UINT64 rational,
current 1 second, with 55 choices ranging from 1/8000 through 30 seconds. No
property value was written. The identical result passed in two power cycles;
the lifecycle repetition gate remains open.

## Deliberately withheld widgets

- Extended ISO `0xd01e` (UINT32 on the observed body, but its empty enumeration
  and numeric meaning require client correlation before any write)
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

## Hardware enactment checklist

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
