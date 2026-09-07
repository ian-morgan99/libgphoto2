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

## Source-repository ownership rule

A failure observed through Benro Polaris, OpenPolaris, another appliance, or another
consumer is **discovery/integration evidence only** until it reproduces at this
repository's own boundary.

Before changing libgphoto2 for a consumer-discovered failure:

1. build an exact clean libgphoto2 SHA in an isolated prefix;
2. attach the affected camera directly to the host;
3. prove the running `gphoto2`/test harness loads that exact library and camlib;
4. reproduce from a fresh camera/session;
5. capture `gphoto2 --debug` or the smallest repository-local harness trace;
6. identify the first failing PTP operation/state transition, not just the final GP error;
7. add a deterministic regression test where practical.

Interpretation:

```text
direct libgphoto2 FAIL + consumer FAIL at same lower-level operation
  -> candidate libgphoto2 defect

direct libgphoto2 PASS + Polaris/runtime FAIL
  -> consumer/runtime/integration defect; do not change libgphoto2
direct libgphoto2 PASS + Polaris PASS + OpenPolaris FAIL
  -> OpenPolaris/client defect
```

A consumer PASS does not by itself prove or close a libgphoto2 issue, and a
consumer FAIL does not justify a library workaround.

## Generic PTP2 and other-camera regression gate

Pentax work must not regress cameras already supported by libgphoto2 or by known
consumers. Any change that touches generic PTP2 code, port/session lifecycle,
shared config helpers, transfer logic, timeouts, error handling, filesystem
state, or model-independent behavior requires an explicit impact statement:

```text
Pentax-only path? yes/no
Generic PTP2 path changed? yes/no
Other vendors potentially affected? list
Why model/vendor gating is sufficient, or why behavior is genuinely generic
Automated regression coverage added
Physical/non-Pentax evidence available
```

Rules:

- Prefer vendor/model/capability gating for Pentax-specific recovery/workarounds.
- Do not alter generic semantics merely because a Polaris integration requires it.
- Preserve the previous code path for non-Pentax cameras unless a generic fix is
  independently justified and tested.
- If a generic path changes, run the normal upstream suite and targeted tests for
  representative non-Pentax behavior before hardware qualification.
- Consumer projects must rerun their own previously-qualified camera matrices
  before adopting a new libgphoto2 SHA. For the Benro Polaris consumer, the
  normative appliance process is `docs/LIBGPHOTO2-UPGRADE-PROCESS.md` in
  `ian-morgan99/benro-polaris-firmware-patcher`.

## Current Polaris integration lesson (2026-09-07)

The K-3 III preview case is a reference ownership example: the same exact
libgphoto2 SHA produced a valid preview through direct `gphoto2` on the Polaris,
while the packaged `pgphoto` / Stage-2 path failed. That differential points to
packaging/runtime integration until proven otherwise. Do not introduce a Pentax
library change for that symptom unless a clean direct libgphoto2 reproducer is
subsequently obtained.

## Current K-1 Mark II setting finding

On firmware 1.02 in PC-P Manual mode, conditions reported raw mode 8, idle, and
capability flags `0x0000000f`. Source-faithful long-lived tests proved restored
round trips for shutter 1/500→1/125→1/500 and ISO 200→400→200.

The crucial rule is that IT2 obtains live exposure values from periodic
`GetAllConditions`; a descriptor's `CurrentValue` can lag and must not be the
sole post-write oracle. Reproduce IT2's initial conditions load, second 100 ms
poll, serialized write, and bounded later condition polls. Still require exact
restoration and an independent fresh-session check.
