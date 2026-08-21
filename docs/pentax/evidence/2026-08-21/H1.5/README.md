# H1.5 — bounded K-3 III focus-drive probe

Camera: K-3 Mark III firmware 2.20 with a 24–70 mm lens, physical exposure
mode `M`, focus selector set to AF.

## Source contract

The decompiled IMAGE Transmitter 2 K-3 III path reads `openAvNum` from
little-endian byte offset 328 of `GetAllConditions` (`0x900f`). Its minimum
Near command sends positive `(openAvNum * 5) / 6` as the sole parameter of
`0x9017`; Far sends the exact negative. The client has an escalating retry loop
for response `0xa00c`. The candidate deliberately implements no retry or
escalation.

No lens-name/model field was found in the extracted managed remote-control
source. Lens metadata may be supplied by another component or image metadata;
it is not an input to the observed K-3 III focus formula.

## Host validation

- clean Meson 1.12 build: pass;
- `test-pentax-utils`: pass, including +29/-29 for `openAvNum=35`, invalid
  direction, and zero-magnitude rejection;
- purpose-built `pentax-safe-focus` harness: pass compilation;
- exact model, explicit USB port, one direction per process, 20-second outer
  timeout, normal exit cleanup, and no retry path.

## Hardware result

The camera was exclusively available as `25fb:0189` at `usb:001,010`.

1. Two preliminary container runs returned `GP_ERROR_MODEL_NOT_FOUND` during
   host-side detection. The camera module had not loaded because runtime
   `libxml2` was missing. Neither run opened a PTP session or sent a camera
   command.
2. With the complete runtime, the first camera-reaching minimum Near run
   returned `GP_ERROR_IO`; cleanup was attempted.
3. The required success condition was not met, so no Far command and no retry
   was sent.
4. The operator subsequently confirmed that the lens neither moved nor made an
   audible movement sound and that it was set to AF during the probe.
5. One diagnostic Near repeat was authorized after stage-specific raw response
   messages were compiled. It again returned `GP_ERROR_IO`, no Far command was
   sent, and the operator was asked to observe movement. None of the setter's
   compiled `0x900f`/`0x9017` error messages appeared. This indicates the error
   occurred before those instrumented calls, but the first harness version did
   not label camera-init versus action-widget retrieval.
6. The operator confirmed there was no audible lens movement during the second
   attempt.
7. The final stage-labelled Near diagnostic returned `GP_ERROR_IO` at
   `camera-init`. The harness therefore never retrieved the focus action and
   never entered the instrumented `0x900f`/`0x9017` setter. No Far command was
   sent and the focus sequence was closed.
8. The camera was powered off until `25fb:0189` disappeared, with no remaining
   MTP mount or test process. After a user-confirmed normal shooting-screen
   restart it re-enumerated as `usb:001,011`. A newly compiled `init` mode then
   performed only explicit camera open, initialization, and intended cleanup;
   it again returned `GP_ERROR_IO` at `camera-init`. It never retrieved or
   invoked a focus action.

Status: **BLOCKED AT SESSION INITIALIZATION / STOPPED**. The stage-labelled
result establishes that the current failure precedes the focus action; it is
not a hardware rejection of `0x9017` or validation of offset 328. Operator
observation establishes no physical movement in the first two attempts. Focus
testing must not resume until lifecycle/recovery investigation restores a
successful fresh initialization.
