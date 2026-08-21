# Pentax development instructions

These instructions apply to all work in this repository.

## Normative protocol source

Treat the bundled decompiled IMAGE Transmitter 2 implementation as the
normative statement of how Ricoh/Pentax intended supported cameras to be
controlled. Before implementing, changing, or testing a Pentax opcode,
property, payload, model gate, state transition, retry, or cleanup path:

1. Trace the complete IMAGE Transmitter 2 call path, including its model gate,
   UI gate, value list, serialization, transport phases, response handling,
   condition polling, and teardown.
2. Record the source anchors and distinguish direct source evidence from an
   inference. Do not infer support from names, adjacent codes, DeviceInfo, or a
   GetSet descriptor alone.
3. Use `docs/pentax/IMAGE_TRANSMITTER_CAPABILITY_MATRIX.md` as the target matrix,
   `docs/pentax/IMAGE_TRANSMITTER_SETTING_PATH.md` as the setting-write baseline,
   and `docs/pentax/REAL_HARDWARE_TEST_LOG.md` as the canonical hardware record.
4. Update `DEVELOPMENT_PLAN.md` and the separate hardware log after every real
   camera test. Preserve exact original values and restoration obligations.

## Hardware truth rules

- A PTP `0x2001` response proves acknowledgement, not application. A setting
  write passes only after changed-value read-back and exact restoration are
  independently verified.
- Run one variable per hardware gate. Read the current value and descriptor,
  choose only an advertised value, preflight source-defined conditions, stop on
  the first mismatch, and restore on every branch where the value changed.
- Never guess or sweep vendor opcodes, property values, focus magnitudes,
  cancellation commands, or recovery sequences.
- Keep preview frames in memory unless an evidence plan explicitly requires a
  disposable file. Never capture, delete, reset, shut down, or modify firmware
  merely to diagnose another feature.
- Fail closed when vendor enable, parsing, ownership, verification, or cleanup
  fails. Do not hide a failure with a later successful cleanup response.

## Current K-1 Mark II setting finding

On firmware 1.02 in PC-P Manual mode, conditions reported raw mode 8, idle, and
capability flags `0x0000000f`. Source-faithful long-lived tests proved restored
round trips for shutter 1/500→1/125→1/500 and ISO 200→400→200.

The crucial rule is that IT2 obtains live exposure values from periodic
`GetAllConditions`; a descriptor's `CurrentValue` can lag and must not be the
sole post-write oracle. Reproduce IT2's initial conditions load, second 100 ms
poll, serialized write, and bounded later condition polls. Still require exact
restoration and an independent fresh-session check.
