# K-1 II session safety: start here

Updated 2026-10-30. This is a host-library handoff, **not a firmware release**.
Do not describe a compile or simulated camera as a physical PASS.

## Exact repositories and ownership

| Work | Checkout / branch | Status |
|---|---|---|
| Library disconnect and reconnect safety | `/home/ian/Documents/VSCodeProjects/LibGphoto2-v86-main`, `main` | Work directly here at the operator's request. Disconnect fix `6fe00aeb68d8b98a9e93dfb8a9cad989f8e0626e`; the following session-barrier commit contains this handoff. Resolve its full SHA with `git log -- docs/pentax/K1II-SESSION-SAFETY-HANDOFF.md`. |
| Patcher diagnostics/readiness | `/home/ian/Documents/VSCodeProjects/LibGphoto2/polaris-lag-review`, `fix/k1ii-control-latency` | Clean at `322a913`; earlier code commits `efeaa08`, `df50c1c`. Not merged, packaged or deployed. Its origin is a **local checkout**, not GitHub. |
| Separate Bulb investigation | `/home/ian/Documents/VSCodeProjects/LibGphoto2/polaris-bulb-review`, `fix/k1ii-k3iii-lifecycle` | `c8fa429`, unfinished; do not import as a fix. |
| Other patcher work | `/home/ian/Documents/VSCodeProjects/BenroPolarisPatcher`, `feature/astro-native-platesolve` | Unrelated dirty/concurrent source, release-process and documentation changes. Preserved; not safe to clean, commit or reset on this task's authority. |
| Older library checkout | `/home/ian/Documents/VSCodeProjects/LibGphoto2/libgphoto2`, `feature/polaris-capture-events` | Not the active main checkout. Do not build it by mistake. |

No worktree, old branch, evidence or unrelated modification was deleted.
No subagents, paid service, package installation or camera operation is needed
to repeat the offline checks below.

## What the new library work changes

1. Ordinary disconnect no longer requests CameraShutdown (`0x9002`). Its
   vendor-disable and session-close errors remain visible; see
   [the disconnect audit](PENTAX-DISCONNECT-SAFETY.md).
2. Both `SessionAlreadyOpened` and the existing `0x02fa`/transport-error
   reconnect routes now require successful d035 and complete conditions
   observations before closing/resetting. Active/unknown live view, existing
   unsafe activity bits, pending candidates, unreadable/short data, an earlier
   recovery barrier, and non-USB transports refuse recovery.
3. The previous idle CloseSession → USB port reset → two-second settle →
   OpenSession sequence is retained, but stops on the first failure and must
   pass a second state observation. A failed reopen cannot fall through as
   success. Pentax no longer falls into an unobserved generic class-reset
   after invalid transaction IDs. Non-Pentax retry/reset paths are unchanged.
4. Observations no longer clear `vendor_mode_unknown`; successful vendor
   enable does. Failed early initialization releases host allocations without
   sending camera teardown commands. The core closes the host port on failure.

This is a safety fix for source-confirmed defects, **not a proven lag fix**.
There is no new periodic poll, setting, capture, focus, Bulb or format change.
Failure is intentionally more conservative: a pre-enable observation that a
body refuses now prevents destructive recovery, even if recovery used to
continue. Real K-1 II/K-3 III reconnect compatibility needs qualification.

### Source versus inference

Normative IT2 2625 `RemoteAssistant/MtpDevice.cs` hash and full disconnect
path are in the linked audit. `Connect`, `TimerInitialize`, `VendorMode`,
`Disconnect`, `PcLiveViewMode`, `MtpGetAllConditions` and the serialized WPD
read/no-data transport paths are the source anchors. Conditions use UINT32
fields at offsets 32/36 for the transfer candidate and 104 for activity;
`PcLiveViewMode` writes byte 0/1 to d035. Existing library reconciliation
reads that property as UINT8 and requires at least 508 condition bytes.
The seven existing named unsafe activity flags are unchanged, not a claim
that every firmware state has been independently audited.

IT2 manages sessions through WPD and does **not** specify our USB reset
recovery. That sequence is retained from appliance evidence, not attributed
to IT2. USB exclusivity relies on public `gp_camera_init` successfully opening
the port before entering camlib initialization, and libusb's successful
interface claim (`gp_libusb1_open`). A failed claim never reaches camlib init.
This does not establish exclusive ownership for PTP/IP or raw direct calls
to the camlib entry point; those are not supported recovery test shortcuts.

## Repeatable host build (no camera I/O)

From clean active main, run:

```sh
cd /home/ian/Documents/VSCodeProjects/LibGphoto2-v86-main
bash tests/run-pentax-host-check.sh
```

The script refuses dirty/non-main source, uses installed tools only and
disables downloads/research capture. It creates a unique
`/tmp/pentax-host-check.*` directory containing the source SHA, configure/build/
test/install logs, an isolated prefix and installed-file hashes. It does not
overwrite system libgphoto2, contact Polaris or run gphoto2 against a camera.
Retain the printed directory and SHA with the issue; `/tmp` is not durable
release storage. Never add `--suite no-ci`: that suite contains manual tests.

Current regression evidence: 20/20 host tests pass. The startup fixture
extracts the actual production observer, failure cleanup, recovery helper and
OpenSession loop; 84 cases cover seven unsafe activity bits across all four
stale/error responses, short/failed reads, live view, candidates, failure
ordering, post-reopen refusal, host cleanup and generic-camera controls.
The earlier actual-exit fixture covers 31 scenarios. The new fixture passes
ASan/UBSan and warnings-as-errors; its ARM EABI5 object compiles with the
installed `arm-linux-gnueabi-gcc`. This is not a complete ARM library build,
ARM execution, all-camlib build, autotools test run or hardware qualification.

## Remaining gates, in order

1. Independent review on [libgphoto2 #33](https://github.com/ian-morgan99/libgphoto2/issues/33)
   and [#138](https://github.com/ian-morgan99/libgphoto2/issues/138): inspect the
   production diff, ownership invariant, conservative pre-enable refusal,
   error/cleanup behavior and test limits. Do not close either from host tests.
2. Direct-PC K-1 II clean-session init/read/exit, then a separate warm reconnect
   and K-3 III equivalent, from a clean SHA and proven core/camlib/iolib loading.
   **Do not run a generic gphoto2 settings command as an unattended read-only
   test yet:** idle recovery still contains a USB reset. The test plan must
   exclude recovery/reset as a diagnostic action and stop on a stale session.
   No PTP session, setting, capture, preview or reset was performed here.
   The old `ptp-probe.c` in the other checkout is unsuitable: it uses guessed
   layouts and sweeps vendor operations despite its read-only description.
3. Only after init/cleanup qualification, read one settings tree and time
   individual controls, including EV/WB/shutter/format. Stop on the first
   mismatch. Follow the conditions-based readback and exact-restoration rules
   before any separately approved setter. Update DEVELOPMENT_PLAN and the
   hardware log after each real test, with firmware, exact SHA and raw trace.
4. Reproduce the lag at the correct boundary. Patcher
   [#253](https://github.com/ian-morgan99/benro-polaris-firmware-patcher/issues/253)
   remains open. Its saved handoff is
   `docs/evidence/k1ii-latency-followup-20261030.md` in the latency checkout.
5. Resolve patcher [#251](https://github.com/ian-morgan99/benro-polaris-firmware-patcher/issues/251)
   separately. The existing v90 Bulb cancel/deferred-capture release check is
   deliberately RED. Do not weaken it to obtain a package. Only then use the
   documented release process with exact cross-repo SHAs, published ZIP,
   Bluetooth wake/install, runtime provenance and physical camera matrix.
   Never copy replacement binaries onto Polaris over SSH.

## Shared-code impact

```text
Pentax-only behavior change? yes, existing supported-PID/USB-vendor gate
Generic PTP2 code touched? yes, initialization and ordinary exit dispatch
Other vendors potentially affected? all PTP2 vendors share these functions
Protection: old generic retry/reset and exit semantics retained; actual-loop
            and actual-exit negative controls exercise those branches
Automated coverage: 20-test host suite, 84 startup cases, 31 cleanup cases
Physical/non-Pentax evidence for this revision: none
```
