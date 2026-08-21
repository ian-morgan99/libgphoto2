# H1.12 — K-1 Mark II single-preview attempt

Camera: PENTAX K-1 Mark II firmware 1.02 in PC-P mode. Serial omitted.
The operator reported after the attempt that no memory card was installed. A
card was inserted afterward. Card absence is therefore an uncontrolled
precondition and H1.12 cannot distinguish a camera-readiness refusal from a
live-view implementation failure.

The current source was built in a fresh isolated container with the explicit
research-capture compile flag. `ptp2`, the dedicated safe-preview harness,
`test-pentax-utils`, and `test-camera-list` built; both tests passed. The harness
was extended to require one of the two exact supported Pentax model names.

The hardware invocation selected the exact K-1 II model and USB node, requested
one frame, ran under a 45-second timeout, retained no image, and exposed no
configuration, capture, transfer, delete, or reset action. Vendor enable
succeeded with function flags `0x00000003`. The composite capture-preview path
returned `GP_ERROR_IO` before a valid JPEG was available. No retry was made.

The harness executed `gp_camera_exit` on the error path and the USB node was
unowned afterward. Because the current diagnostic does not separately report
the `0xd009` read, `0xd035=1`, `0x9006`, `0xd035=0`, and vendor-disable results,
this attempt does **not** prove which substage failed or independently prove the
PC-live-view restoration response.

An operator-requested retest was then performed with a memory card installed.
After gracefully releasing the desktop GVFS camera claim, the same exact-model
one-frame harness ran directly from the already-built artifacts. Vendor enable
again succeeded with flags `0x00000003`, and the composite preview path again
returned `GP_ERROR_IO` before a JPEG. No retry followed. The USB node was
released. This rules out card absence as the sole cause.

Status: **REPRODUCIBLE FAIL-CLOSED / INSTRUMENTATION REQUIRED**. Before another preview
attempt, add stage-specific raw response logging and an exit report that
preserves the primary failure while listing every restoration/cleanup result.
Then rerun exactly one frame; do not advance to 10 frames until it passes.
