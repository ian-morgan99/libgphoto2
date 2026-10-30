# Clean host build and USB-only evidence, 2026-10-30

Source: `f97110d9272e24cf242830e83b4105092a0fdde4`, clean `main` at build.
Command: `bash tests/run-pentax-host-check.sh` from the active checkout
`/home/ian/Documents/VSCodeProjects/LibGphoto2-v86-main`.
Output: `/tmp/pentax-host-check.1eTacOnD`, install under its `prefix`.

- Configure/build/install completed, using installed tools only, research
  capture disabled and Meson downloads disabled. Existing compiler warnings
  are retained in `build.log`; this is not a warning-free whole-library claim.
- `tests.log`: 20/20 pass, manual `no-ci` suite excluded.
- `sanitizers.log`: 84 startup-loop cases pass ASan/UBSan including leak
  detection, compiled with `-D_GPHOTO2_INTERNAL_CODE -std=gnu99 -g -O1
  -Wall -Wextra -Werror -Wno-unused-parameter -fsanitize=address,undefined
  -fno-omit-frame-pointer`, the existing Meson build's headers/generated
  fixture, `test-pentax-session.c` and `pentax-utils.c`, linking host core/port.
  This sanitizes that fixture/utilities, not the complete linked core. The
  private-structure free is counted by the fixture; real observation buffers
  are allocated/freed. Not a full `gp_camera_init` memory/ABI test.
- `arm-object.log`: compile-only ARM EABI5 object of the same fixture using
  installed `arm-linux-gnueabi-gcc` 13.3.0. No ARM execution, linked library,
  firmware build or deployment.
- `installed-sha256.txt`: hashes of the installed files; preserve source SHA
  and paths together. This directory contains logs, **not** firmware binaries.

At 16:22:21 UTC the physical K-1 II enumerated as `25fb:0183`, `usb:001,006`.
The USB-only command was:

```sh
env -u LD_PRELOAD \
  LD_LIBRARY_PATH=/tmp/pentax-host-check.1eTacOnD/prefix/lib \
  CAMLIBS=/tmp/pentax-host-check.1eTacOnD/prefix/lib/libgphoto2/2.5.34 \
  IOLIBS=/tmp/pentax-host-check.1eTacOnD/prefix/lib/libgphoto2_port/0.12.2 \
  LD_DEBUG=libs LD_DEBUG_OUTPUT=/tmp/pentax-host-check.1eTacOnD/loaded-libraries \
  timeout 30 /usr/bin/gphoto2 --debug \
  --debug-logfile=/tmp/pentax-host-check.1eTacOnD/usb-detect-debug.log --auto-detect
```

`usb-detect-debug.log` records USB descriptors/abilities only; no camera init
or OpenSession. `loaded-libraries.log` is the corresponding gphoto2 process
loader trace and proves the prefix's core, port, ptp2 and usb1 libraries loaded.
No serial number or image was collected; firmware was not queried. No settings,
capture, focus, live view, shutdown or reset was requested. This result does
not qualify the new cleanup/recovery code, lag, or consumer behavior.

Issues #33/#138 remain open for independent review and the explicitly bounded
direct-session/reconnect gates described in the main handoff. Patcher #253's
lag and #251's Bulb release blocker remain separate. No package was built.
