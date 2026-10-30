# Pentax ordinary disconnect safety — issue #138

Date: 2026-10-30. Source-level repair on `main`, explicitly requested by the
operator. No new branch, firmware build, deployment or camera action.

## Scope and source evidence

At baseline `63a894b0821a510fc40244c5cc4ef84a5220a91c`, `camera_exit()`
sent CameraShutdown (`0x9002`) after successfully disabling vendor mode.
The call came from `8ba60c3e7c60d9bab403a19711202282f12f8a03` as a
best-effort stale-session workaround. The 2026-08-26 historical hardware log
records that the recovery experiments still failed; it does not establish
that automatic shutdown was a successful or source-faithful remedy.

Normative reference: IT2 2625 `RemoteAssistant/MtpDevice.cs`, SHA-256
`e8d11ed772aad3731d7eabd0c06fb0174aae859c5a1f6d241bf73b6d046de059`.

- `Model` assigns K-1 II 78400/extension 1. The ordinary disconnect path is
  shared by its recognised models; it has no model-specific shutdown branch.
- `Connect()` opens WPD, enables vendor mode, reads DeviceInfo, storage IDs
  and conditions. `TimerInitialize()` starts condition polling after 100 ms.
- `Disconnect()` at line 3628 disarms/disposes both timers, sets
  `VendorMode=false`, and closes WPD. `Dispose()` calls that same method.
- `VendorMode` at line 923 calls `MtpSetPentaxVendorMode` at line 4617:
  no-data `0x9001`, five UINT32 parameters
  `[13, model, 0, extension, 0]`. No setting value list or payload applies.
  `ExecuteCommandWithoutDataPhase` serializes under `_wpdCmdLock` and calls
  WPD's no-data command path, retrieving the response code/parameters.
  Transport exceptions produce response zero; a non-`0x2001` disable throws.
  `Disconnect()` catches that exception and still closes WPD.
- Shutdown is separate: `RequestCamShutdown()` at line 3762 and the explicit
  `IsRequestShutdown` setter call `MtpPentaxCameraShutdown()` at line 4623:
  no-data `0x9002`, no parameters. `RequestCamShutdown()` returns false for
  `0x2019`; otherwise IT2 continues to disconnect. This does not establish
  physical power-off for every response or model.
- `MainWindow.DisconnectDevice()` at line 2422 calls ordinary disconnect.
  Only its separate `CamShutDwnBtn_Click()` at line 6333 first requests camera
  shutdown. `RemoteAssistant/MainWindow.cs` SHA-256:
  `0710e0174877f644abd689f98cb7df6f1c1111b2300e3fc1985002f4e2b4822c`.

The source proves that ordinary disconnect must not request shutdown. It does
not prove a cause of Polaris lag, overheating, app crashes or reconnect failure.

## Implementation

- Remove only the automatic `0x9002` call from the Pentax exit branch.
- Retain forced restoration of owned live view, vendor-mode disable, event
  draining, ordinary PTP CloseSession and resource cleanup. Do not add a
  replacement reset, retry, polling loop, setting or shutdown control.
- Report a failed Pentax CloseSession instead of silently returning success.
  Live-view restoration errors still take precedence over vendor-disable
  errors, which take precedence over CloseSession errors. Resource cleanup
  still runs. This is deliberately stricter error reporting than IT2.
- Preserve the existing `DONT_CLOSE_SESSION` guard and non-Pentax semantics.

## Regression evidence and limits

`tests/extract-camera-exit.py` extracts the actual `camera_exit()` and error
translation functions without changing their bodies. The Meson test compiles
them with real camlib structures and mocked transport/cleanup helpers. This
avoids an installed test-only ABI entry point or a hand-rewritten cleanup
algorithm. No USB device is opened by the fixture.

`test-pentax-camera-exit` covers 31 scenarios: the 11 recognised modern model
IDs (including extension 0), owned/unknown/disabled vendor mode, live-view
restoration including keep-live-view, failed restoration/disable/close and
first-error precedence, generic DeviceInfo vendor with enabled Pentax mode,
the no-close guard, uninitialised cleanup, and Canon/Nikon/Sony/Microsoft
negative controls. It asserts the complete ordered mock operation list;
an unexpected shutdown fails even when its simulated response is success.

The original `63a894b08` cleanup body fails this fixture at the operation-count
assertion because it sends the extra shutdown command. The repaired source
passes. This is execution of the real cleanup control flow, not physical
transport or ABI qualification of a running camera session.

The final 31-scenario fixture also passes a separate `-Wall -Wextra -Werror`
build (unused mock parameters excepted) with AddressSanitizer leak detection
and UndefinedBehaviorSanitizer, both configured to stop on error. This
sanitizes the extracted cleanup fixture, not every linked core-library object.

Host configuration includes `ptp2,directory` camlibs and `libusb1,disk` iolibs,
with research capture disabled and `--wrap-mode=nodownload`. The ordinary
host suite passes **19/19**, excluding the manual `no-ci` serial-port test.
The fixture is registered in Meson; its source/extractor are also distributed
in autotools tarballs, but this change does not claim an autotools test run.
Raw configure/build/test, negative-regression and sanitizer outputs are saved
under `evidence/2026-10-30/issue-138-disconnect/`.

Reproduce with installed tools only (never run the manual `no-ci` suite with
an unattended camera attached):

```sh
meson setup /home/ian/Documents/VSCodeProjects/LibGphoto2/build-pentax-cleanup-main \
  /home/ian/Documents/VSCodeProjects/LibGphoto2-v86-main \
  --prefix=/home/ian/Documents/VSCodeProjects/LibGphoto2/prefix-pentax-cleanup-main \
  --libdir=lib --wrap-mode=nodownload -Dcamlibs=ptp2,directory \
  -Diolibs=libusb1,disk -Ddocs=false -Dpentax_research_capture=false
ninja -C /home/ian/Documents/VSCodeProjects/LibGphoto2/build-pentax-cleanup-main -j4
meson test -C /home/ian/Documents/VSCodeProjects/LibGphoto2/build-pentax-cleanup-main \
  --no-rebuild --no-suite no-ci --print-errorlogs
```

For an already configured directory omit `meson setup`. No install command
is needed for this offline test run.

## Required impact statement

```text
Pentax-only path? yes (both behavioral changes)
Generic PTP2 path changed? yes (capture CloseSession result; apply only to Pentax)
Other vendors potentially affected? shared function includes all PTP2 vendors
Gating: error result is used only when the Pentax switch branch was selected;
        other vendor cleanup and return behavior, including no-close, is unchanged
Automated coverage: actual-function fixture; non-Pentax negative controls;
                    complete hardware-free Meson suite
Physical/non-Pentax evidence: none for this revision
```

## Still open — do not turn this into a camera-readiness claim

**Follow-up 2026-10-30:** the separate initializer barrier is now implemented
and covered by 84 actual-startup-loop fault-injection cases. The combined host
suite passes 20/20. See [the current handoff](K1II-SESSION-SAFETY-HANDOFF.md)
for exact checkout/build commands and remaining gates. The paragraphs below
describe the scope of the earlier disconnect-only commit `6fe00aeb6`.

Issue [#138](https://github.com/ian-morgan99/libgphoto2/issues/138) remains open
for review and physical qualification. This slice does **not** alter the
separate stale-session initializer, which may reset USB after an unsafe or
unreadable observation. Its #33/#30 ownership and recovery barriers need their
own narrowly scoped review/tests before an unattended read/init/exit probe can
be described as guaranteed read-only. Do not bypass that guard by skipping
cleanup or substituting successful transport responses.

K-1 II and K-3 III fresh/warm reconnect behavior remains a hardware gate.
No capture, Bulb, focus, setting-write, thermal or Polaris-lag result is claimed.
Consumers must use the documented firmware ZIP path and rerun their camera
matrix before adopting the new library SHA; nothing has been deployed here.
