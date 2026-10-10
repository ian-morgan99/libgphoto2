# K-3 III B-mode Bulb diagnostic — 2026-10-10

## Result

Four direct-PC, one-second held-Bulb captures completed on the K-3 III across two two-shot sessions without a camera reboot. All start/stop edges were accepted and each shot produced DNG/JPEG output. The final two-shot session used the product-specific K-3 III gate now proposed for issue #95; EXIF times were 1.068 and 1.064 seconds. This is **Layer A diagnostic evidence**, not firmware or release qualification.

- Camera: K-3 III, USB `25fb:0189`, direct PC, fresh port `usb:001,015` for the final two-shot run.
- Source base: libgphoto2 `6b4b3b935ccfde4b3c8a25b1ed131f0a68f70832` (main after PR #98).
- Before each final run: B mode (`exposure-mode-raw=9`), camera Bulb timer off, idle with no shooting/processing/task-changing flags.
- Research capture enabled. The qualification build enabled the standard K-3 III USB product `0x0189`; product `0x018f` (Monochrome) remains blocked. No Polaris firmware was changed.
- For both final shots, `0x9011`/release=2 and matching `0x9012`/release=2 returned `0x2001`; libgphoto2 completed candidate transfer/publication.
- The first `IMGP3900/3901` pair came from a diagnostic model-only gate override. The final `IMGP3902/3903` pair used the product-specific `0x0189` gate now proposed in this issue branch; `0x018f` was blocked.
- Saved outputs and EXIF `ExposureTime`: `IMGP3900` 1.073 s, `IMGP3901` 1.066 s, `IMGP3902` 1.068 s, `IMGP3903` 1.064 s. DNG/JPEG pairs agree. The image bytes remain local under `/tmp/k3iii-bulb-issue95-outputs/`; hashes are listed below, not committed.
- A write attempt to set `/main/status/pentaxdirectshutter=00:01` returned `GP_ERROR_NOT_SUPPORTED`; read-back remained `00:00`, with `bulb-timer=no`. No setting change occurred. The successful test used the application-held start/stop action, not the camera-owned timer.

## Admission-path finding

At initialization, the camlib could have `recovery_required=1` despite readable idle conditions and no pending output. Before PR #98, the action wrapper returned `GP_ERROR_CAMERA_BUSY` before the shared capture routine could run its strict recovery probe. PR #98 is now merged; the final two-shot run began with `recovery=1`, the strict probe cleared it, and both captures completed. The standard K-3 III product gate was exercised in the issue #95 worktree, with the Monochrome product explicitly blocked.

The 2026-09-06 `0x2002` result was recorded in Manual (`exposure-mode-raw=8`), not B mode. It did not test the B-mode release-2 sequence selected by IMAGE Transmitter 2 when the camera's own Bulb timer is disabled.

## Raw transcripts and hashes

The tracked PTY transcript is normalized from CRLF to LF; the original capture
remains in `/tmp`.

- `k3iii-bulb-20261010-pc-test.txt` — first action attempt returned `-110` before entering the camlib lifecycle. SHA-256 `29077a8434eb93139839cf00214b507c94276ddd222b6c4f65f94929767f3dcc`.
- `k3iii-bulb-20261010-pc-test-retry.txt` — the stale `recovery_required` guard refused before `0x9011`. SHA-256 `da29f080f4df490688ef71a43f6d2590aad325f82bd274ce8e4f6450b4b3c948`.
- `k3iii-bulb-20261010-pc-test-final.txt` — first diagnostic one-shot, with file bytes not saved. SHA-256 `ca2077b50d77fcd77ccd992b4ebb8162bf2c1694ae26fb7565a766656b44b9df`.
- `k3iii-bulb-issue95-two-shot.txt` — final two-shot run, including saved-output events. SHA-256 `b2d3ebfd4cd170bda6bffa329011e6b93463e157bdc1fea0d26acec0cc84325d`.

The manual, research-build hardware probe is [pentax-bulb-qualification-probe.c](../../../../../tests/pentax-bulb-qualification-probe.c), SHA-256 `9417776d6d503b0e9b79c37a20b039914f682b6f95822fd30f9fd19853784067`. The tested `ptp2.so` SHA-256 is `1520766769cbc5d1aefdfd72d199b153853278dc943488dd91452535e83ba0fa`; probe binary SHA-256 is `769e798f137e36593dcd6416405adc1b3ec21e2f4496f6795db9ecd177890b52`. The Patcher firmware stack was not built or loaded.

The four captured image files remain local, not committed, because they contain camera imagery. Their SHA-256 values are:

- `IMGP3900.DNG`: `63b2ac8ecc73e7b6b7456a41e265324945889d37f6e205cc0eb96f42bf698df0`
- `IMGP3900.JPG`: `831aacfecaea45e52c32a64fc2e2161a4ab8a3fca20f92c1cdd839978cf62db`
- `IMGP3901.DNG`: `dacd83000b36ec750e81cd9d356655c845a7dc78052c93fd101d6718fda8f8ee`
- `IMGP3901.JPG`: `5f1f4ef87e8dace66d30072033256562752475578c118d34f8dd5c89d8a79133`
- `IMGP3902.DNG`: `fcfce80f9bd83ff5dd8a87f33e03f53bd38a9b546c89251d1f25da6be38b392e`
- `IMGP3902.JPG`: `1dea7f9cde20f32c5761c52e9d72693c567bc7167cf8eefac2bad533ca9ff8dc`
- `IMGP3903.DNG`: `92be3c53cea8230a34667b57e0d068e016dd032a01dc0a100149562089a21a68`
- `IMGP3903.JPG`: `2bfb9e7802da2d9afd480c81ad025d2e53a6df424244fef08c179a5cd3c20d6c`

## Still required before closing #95

PR #98's recovery-admission fix is merged. The product-specific K-3 III gate
and its tests are in the current issue95 branch but are not yet merged. Direct
PC held start/stop now has two consecutive sessions, saved DNG/JPEG pairs, and
EXIF-duration evidence. Still open: the camera-owned timed/natural-completion
control (the B-mode timer write was rejected), the stop/natural-close race, a
subsequent Manual control capture, and wiring pgphoto's code-264 Bulb request to
this action. The current Benro firmware does not do that: `bulb:N` is a wait
limit, and stock `captureBulbImage` has no callers. No firmware candidate was
built, installed, or qualified.
