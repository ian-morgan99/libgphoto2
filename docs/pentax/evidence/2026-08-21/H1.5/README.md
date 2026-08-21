# H1.5 — bounded K-3 III focus-drive probe

Camera: K-3 Mark III firmware 2.20 with a 24–70 mm lens, physical exposure
mode `M`. Camera/lens AF/MF selector state and visible movement were not
recorded before the command and remain required evidence.

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

Status: **INCONCLUSIVE / STOPPED**. The initial build reported only the
translated error and therefore cannot identify whether `0x900f` or `0x9017`
failed. It also cannot establish whether the lens moved before the error. The
next build adds stage-specific raw response reporting. Before another probe,
record the physical AF/MF selector state and the operator's movement
observation.
