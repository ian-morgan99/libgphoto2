# Dual-camera direct-PC verification — 2026-09-11

Source under test: libgphoto2 `1ef21463b` plus the issue #71 working diff.
Bodies: K-1 II `25fb:0183` at `usb:001,047`; K-3 III `25fb:0189` at
`usb:002,009`. The final runs pinned `CAMLIBS` and `IOLIBS` to `_build`.

## Valid final evidence

- `exact-model-preview-and-rejection.log`: the obsolete colon-form model name
  fails with `Unknown model`; exact K-1 II and K-3 III selection then produces
  5/5 valid JPEGs on each body. K-1 II needed 9--13 warm-up polls; K-3 III
  needed one poll per frame.
- `k1ii-exact-focus-final.log`: exact model 78400 dispatches the generic
  near/far aliases to old-focus opcode `0x9016`, one attempt each. Both commands
  returned `0xa00c`; cleanup completed. This proves dispatch, not lens movement.
- `k3iii-exact-focus-final.log`: exact model 78420 dispatches to new-focus
  opcode `0x9017`. Near passed; the immediately following far run hit the known
  transient `0xa008` preview state.
- `k3iii-preview-far-retry-exact.log`: the bounded retry started live view and
  the far `0x9017` command passed with cleanup.

## Invalid/superseded diagnostic runs

The other logs in this directory predate the exact-model probe correction.
They used the colon-form ability names. `sample_open_camera()` incorrectly
returned a stale success value after lookup failure, leaving abilities unset;
with two bodies attached, a different body could then be selected. They are
retained to document discovery of the probe defect and must not be cited as
body-specific hardware evidence.

