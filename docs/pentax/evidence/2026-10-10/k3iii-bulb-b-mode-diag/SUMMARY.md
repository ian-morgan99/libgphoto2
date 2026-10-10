# K-3 III B-mode Bulb diagnostic — 2026-10-10

## Result

One direct-PC, one-second held-Bulb capture completed its start, stop, and libgphoto2 candidate-transfer/publication lifecycle on a Pentax K-3 III. This is a **diagnostic result**, not production qualification.

- Camera: K-3 III, USB `25fb:0189`, PC attachment, port `usb:001,011`.
- Source base: libgphoto2 `67843d2248e7e37cfa58c15cc52e0a47bc13d952` (`.62` source).
- Camera conditions immediately before the capture: idle (`state=0`), B mode (`exposure-mode-raw=9`), camera Bulb timer off.
- Research capture enabled. The diagnostic build temporarily allowed the K-3 III action and removed an early `recovery_required` rejection so the shared strict recovery probe could run. Neither diagnostic override was installed on Polaris.
- `0x9011`/release mode 2 returned `0x2001`; after one second, `0x9012`/release mode 2 returned `0x2001`.
- The shared capture lifecycle published `IMGP3899.DNG` and `IMGP3899.JPG`; the test saw the virtual file count increase from 0 to 2 and exited successfully.

The production libgphoto2 source still blocks the K-3 III held-Bulb action. The test build did not change the camera's menu settings. The output bytes were not saved before the test process exited, so EXIF exposure time was not checked. Do not infer exact-duration accuracy, repeatability, or release qualification from this single run.

## Admission-path finding

At initialization, the camlib had `recovery_required=1` despite readable idle conditions and no pending output. The Bulb action's outer check returned `GP_ERROR_CAMERA_BUSY` before the existing shared capture routine could run its strict recovery probe. After temporarily removing that outer check, the shared probe cleared the recovery state and the B-mode capture completed. This supports a separate narrow fix: let the shared strict recovery path decide whether a pending operation is safe, rather than rejecting solely on the cached recovery flag.

The 2026-09-06 `0x2002` result was recorded in Manual (`exposure-mode-raw=8`), not B mode. It therefore does not test the B-mode release-2 sequence that IMAGE Transmitter 2 selects when the camera's own Bulb timer is disabled.

## Raw transcripts and hashes

The tracked PTY transcripts were normalized from CRLF to LF; the original PTY
captures remain in the local `/tmp` files.

- `k3iii-bulb-20261010-pc-test.txt` — first action attempt returned `-110`; it did not reach the camlib start boundary. SHA-256 `29077a8434eb93139839cf00214b507c94276ddd222b6c4f65f94929767f3dcc`.
- `k3iii-bulb-20261010-pc-test-retry.txt` — confirms the action was rejected by the cached recovery flag before `0x9011`; no shutter start. SHA-256 `da29f080f4df490688ef71a43f6d2590aad325f82bd274ce8e4f6450b4b3c948`.
- `k3iii-bulb-20261010-pc-test-final.txt` — one successful one-second B-mode start/stop and output lifecycle. SHA-256 `ca2077b50d77fcd77ccd992b4ebb8162bf2c1694ae26fb7565a766656b44b9df`.

Diagnostic artifacts (temporary only): `ptp2.so` SHA-256 `da594a8e16fd39e875e82e9265d331f0a5fe5feb38ca94a076b30cc28fac8826`; probe SHA-256 `3a20c0471d454e66eac4acbcb8b6856cf05e95d9c16f5c6ce4ea88dcc690b331`. The source harness is `k3iii-bulb-edge-probe.c` (SHA-256 `0bdd1adf60b7bda6e81d07e3707361a67fcbfe3e4e2d353e73f7981b3b59a3a8`). The temporary source overrides are retained locally at `/tmp/k3iii-bulb-diagnostic-overrides.patch` (SHA-256 `8024724af92511dbdd3bbd01444ad7c3e95573f2396fb8966b1c1ff75a82e61b`); they include a test-only model-gate override, the recovery-path change, and diagnostic trace output. Do not apply that patch as a production change.

## Still required before production support

Follow `ian-morgan99/libgphoto2#95`: save the captured output bytes and verify EXIF exposure duration; test the normal/camera-timed Bulb path and its natural completion first; then qualify held start/stop repeatability and output ownership. Keep the K-3 III production model gate closed until those gates pass. No Polaris reattachment or firmware change was performed.
