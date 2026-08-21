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
| `iso` | ExposureIndex `0x500f` | UINT16 | Existing generic path; hardware unverified |
| `f-number` / `aperture` | FNumber `0x5007` | UINT16 | Existing generic path; hardware unverified |
| `whitebalance` | WhiteBalance `0x5005` | UINT16 | Existing generic path; hardware unverified |
| `focusmode` | FocusMode `0x500a` | UINT16 | Existing generic path; hardware unverified |
| `exposurecompensation` | ExposureBiasCompensation `0x5010` | INT16 | Existing generic path; hardware unverified |
| `shutterspeed` | Pentax/Ricoh `0xd00f` | UINT64 rational | Existing Pentax-vendor path; hardware unverified |

The `0xd00f` constant already existed as `PTP_DPC_RICOH_ShutterSpeed`; the new
Pentax name is an alias for the same wire property. Do not add a duplicate menu
entry.

## Deliberately withheld widgets

- Drive mode `0xd013`
- Exposure-bracketing mode/step `0xd014`/`0xd015`
- Color temperature `0xd018`
- Writing-file-format structure `0xd01b`
- Live-view zoom/mode `0xd036`/`0xd037`

The Windows client establishes that these codes are used, but not enough to
prove each target camera's PTP descriptor type, GetSet flag, enumeration, mode
constraints, or compound payload layout. They remain unexposed until P1 records
real `GetDevicePropDesc` responses and round-trip tests.

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

