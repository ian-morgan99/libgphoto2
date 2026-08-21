# IMAGE Transmitter 2 error and recovery audit

Revision: 2026-08-21

This document records behaviours found in the decompiled IMAGE Transmitter 2
(IT2) client that are easy to miss when reading only its operation and property
tables. They are source evidence about that client, not automatic permission to
copy its recovery policy into libgphoto2. Hardware traces outrank this record.

## Response-specific behaviour

| Context | IT2 response handling | Driver policy |
|---|---|---|
| Enable/disable vendor mode (`0x9001`) | Any response other than `0x2001` is stored as `ConnectResult` and throws. Disable clears its local vendor flag even before throwing. | Fail closed. Never infer that a failed enable entered vendor mode. Cleanup may attempt one disable only when enable was confirmed. |
| Start PC live view (`0xd035=1`) | Only `0x2001` succeeds. Failure raises `LvStartFailed`, leaves PC-LV false, and does not start the frame timer. | Preserve this strict success test. Log the raw response and perform no frame request after failure. |
| Stop PC live view (`0xd035=0`) | Stops polling before the write. Treats `0x2001` and vendor `0xa005` (`NotSupportedMovieMode`) as a completed stop. Any other response restarts polling and raises `LvStopFailed`. | `0xa005` is a narrow, stop-only exception. Do not treat it as general success. Confirm both bodies and modes before public use. |
| Fetch live-view frame (`0x9006`) | A 33 ms periodic callback prevents overlapping requests. Exceptions are swallowed for that frame; polling continues. A non-OK response—including `0xa008` (`NoUpdateImage`)—emits no refreshed-frame event and a later timer tick tries again. | Treat only hardware-confirmed `0xa008` as transient, with attempt and elapsed-time limits plus cancellation. Every other response is terminal. Surface timeout diagnostics, stop polling, and restore state. Infinite silent polling is unacceptable. |
| Set live-view zoom (`0xd037`) | Sends the requested value. Only when 16x receives `0x201c` (`InvalidDevicePropValue`) does it make one fallback write at 10x. Disabling sends magnification 1 and waits 100 ms. Other failures are not surfaced here. | One 16x-to-10x fallback is source-defined but needs a trace and exact restoration test. Never retry other values or responses. |
| New focus (`0x9017`) | Initial signed displacement is derived from open aperture. On `0xa00c` (`FocusControlFailed`) it multiplies the displacement by sqrt(2) and retries, up to ten attempts. | Do **not** adopt this escalation. The fork permits one source-derived minimum displacement and zero automatic retries until a separately approved, hardware-bounded policy exists. |
| Old focus (`0x9016`) | UI Far `+1` becomes amount 5/direction 0; Near `-1` becomes amount 5/direction 1. Mouse-wheel steps use magnitude 10. No response-specific recovery is visible. | Begin with amount 5 only, one direction followed by the equal inverse after physical confirmation. No retry. |
| Camera shutdown (`0x9002`) | Sets disconnect-requested first. `0x2019` (`DeviceBusy`) cancels that state and returns false; every other response returns true. | Keep withheld. IT2 contains no wake command, and accepting every non-busy response is too broad for driver policy. |

## Scheduling and lifecycle behaviour

- Condition polling is a one-shot 100 ms timer. The callback disables the timer
  before `0x900f`, performs any candidate transfer, and rearms only after the
  work completes. This serializes condition reads and transfers.
- A destination-directory failure or local I/O failure raises
  `DestinationFailed` and returns without rearming from that callback. Generic
  transfer exceptions are swallowed and also return. This can silently stop
  condition polling; libgphoto2 must return the error and leave explicit state.
- Connect opens the WPD device and enables vendor mode before reading device
  info, storage IDs, and conditions. If open/enable throws, it closes the WPD
  device. It contains no in-session enable retry and no wake command.
- Disconnect attempts to stop and dispose both timers, then attempts vendor-mode
  disable, swallowing errors in both stages, and finally closes the device.
  libgphoto2 must preserve the primary error while logging cleanup failures.
- The WPD transport distinguishes command-, data-, and response-phase failures
  in `MtpException`. Many higher-level methods discard that distinction by
  throwing a generic exception. Our diagnostics must retain operation code,
  transaction phase, raw response, and lifecycle state.
- Transfer code has separate handling for missing directories, disk-full
  (`NoSpace`), unusable directory destinations, and generic I/O. These are host
  failures, not reasons to retry a camera transaction or acknowledge/delete a
  candidate object.

## Required bounded recovery state machine

1. `generic`: generic PTP session is usable; no Pentax command is assumed.
2. `enabling`: exactly one `0x9001` enable is outstanding.
3. `enabled`: enable returned `0x2001`; vendor operations may run.
4. `live-view`: `0xd035=1` returned `0x2001`; at most one frame request runs.
5. `restoring`: frame polling is stopped before zoom, AF position, and PC-LV
   state are restored in reverse order.
6. `disabling`: issue disable only if enable was confirmed.
7. `failed`: preserve the first error plus every cleanup result; issue no new
   vendor operation in this session.

There is no automatic USB reset, speculative wake command, in-place vendor-mode
retry, focus escalation, capture retry, transfer acknowledgement, or object
deletion. A retry requires a closed session, released ownership, a new device
event, and identity revalidation.

## Junior-agent verification checklist

- Add fixtures before connecting hardware for every accepted exceptional
  response (`0xa005`, `0x201c`, `0xa00c`, and `0x2019`).
- Assert the exact operation count and prove no command follows a terminal
  error.
- For live view, record initial `0xd035`, `0xd036`, and `0xd037`; restore all
  values even after frame failure, while preserving the first error.
- Treat 4-byte `0xd036` as the geometry centre and 8-byte-or-longer values as
  X/Y at offsets 4 and 6. Reject lengths 5–7 and out-of-area coordinates.
- Keep zoom values source-derived (K-body UI lists 2, 4, 8, 10, 12, and 16;
  disable uses 1). Do not infer camera support from the UI list: read/trace each
  body first.
- A public libgphoto2 control needs a documented widget, help text, model gate,
  restoration path, and hardware acceptance evidence. Parser availability alone
  never enables a write.
