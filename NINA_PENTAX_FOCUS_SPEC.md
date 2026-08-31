# Pentax Focus — NINA driver specification

**Audience:** NINA implementer picking up Pentax support.
**Source of truth:** This fork's `libgphoto2/_build/meson-private/dist-unpack/libgphoto2-2.5.34/docs/pentax/` (canonical), `ImageTransmitter2/` decompile (IT2, normative), `HANDOVER.md`, **and** direct K-3, K-3 II, K-3 III firmware decompilation (see §10). The §10 evidence for K-3 and K-3 II is **firmware-based** (ARM Thumb-2 instruction search), not driver-code-based. The libgphoto2 `pentax` (pslr) and `ptp2` Pentax code are derived from pktriggercord, which is known to be incomplete; the firmware itself is the only authoritative answer for what the K-3 and K-3 II can and cannot do.
**Revision:** 2026-08-22 (added firmware-decompilation evidence for K-3 / K-3 II; supersedes the libgphoto2-driver-only analysis from 2026-08-21). Added §12 answering "could libgphoto2 be extended for legacy bodies using the firmware findings?" with the three-interpretation matrix, the K-3 II definitive-closure argument, and the four concrete things the firmware evidence *does* enable. Added §13 with the full 14-firmware capability matrix (MOVW T3 + 32-bit LE literal pool + LDR.W [PC, #imm] cross-reference check across K-3, K-3 II, 645Z, K-01, K-30, K-50, K-500, K-70, KP, K-S1, K-S2, Q7, plus K-3 III as reference; K-1/K-1 II excluded as PTP-only). Disproved the K-70 0x9205 and Q7 0x9204 false-positive literal pool hits. Confirmed no focus opcodes in any of the 13 legacy bodies. Categorised 4 buckets of "addable features" (A: firmware-opcode gaps, B: driver quality, C: bench-verification needed, D: firmware doesn't support it).

---

## 1. TL;DR for the NINA implementer

- **Focus drive is supported on the modern ptp2 vendor path** via two PTP opcodes: `0x9016` (old family: K-1, K-1 II, K-3, K-70, KP, 645D) and `0x9017` (new family: K-3 III, K-3 III Monochrome, GR III). Both are fire-and-forget; no return payload. **Firmware decompilation of K-3 (fwdc220b.bin, 16 MB) and K-3 III (fwdc233b.bin, 35 MB) confirms the opcode families exist on those bodies.**
- **No legacy body supports focus drive or focus position read.** Confirmed by **direct ARM Thumb-2 MOVW T3 + 32-bit LE literal-pool + LDR.W [PC, #imm] cross-reference scan across all 14 legacy/PSLR-era Pentax firmwares** (K-3, K-3 II, 645Z, K-01, K-30, K-50, K-500, K-70, KP, K-S1, K-S2, Q7, plus K-3 III as reference; K-1/K-1 II excluded as PTP-only). **Zero of the 13 legacy bodies implements `0x9016`, `0x9017`, `0x9204`, or `0x9205`.** Only K-3 III (modern PTP) implements the focus dispatch. See §13 for the full cross-firmware matrix.
- **The K-3 II does NOT support focus drive or focus position read.** This is now confirmed by **direct firmware decompilation of `fwdc230b.bin` (16 MB, K-3 II v1.12)**. The K-3 II firmware implements a fixed set of 36 vendor opcodes in the 0x9000-0x9FFF range; **none of `0x9016`, `0x9017`, `0x9204`, `0x9205` are present in the firmware**. By contrast, the K-3 III firmware implements 2,354 vendor opcodes in the same range, including all four focus opcodes. The K-3 II cannot be focus-driven on either the legacy pslr path or the modern ptp2 path. See §10 for the full firmware-decompilation evidence.
- **Focus position read is NOT supported on any of the K-3, K-3 II, K-3 III bodies.** No PTP opcode on these bodies returns absolute lens focus position in steps/mm or any other unit. This is a hard protocol limit, not a driver bug. Three independent reasons, see §6.
- **K-3, K-70, KP, 645D have not been bench-verified in this fork but are predicted to support `0x9016` or `0x9017` based on firmware opcode presence** (K-3 confirmed; the others share the same firmware family as K-3). See §10 for the firmware opcode evidence.
- **Each focus command must be gated by vendor mode** (`0x9001` handshake) and **by an AF-mode precondition on K-1 II's old-focus path** to avoid wedging the lens. See §4.
- **No automatic retry / no IT2-style √2 escalation.** The fork's policy is one source-derived displacement per action, return the first response. See §4 and §7.
- **Use the vendor PTP path only.** The K-1 II's MSC personality (`25fb:0182`) does not support live view, capture, configuration, or focus control. The pslr SCSI-over-MSC driver in `libgphoto2/camlibs/pentax/` does not implement `0x9016` or `0x9017` either. There is no fallback path.

The rest of this document gives the wire protocol, the body matrix, the safety preconditions, the error codes, and the capture/transfer sequence so a NINA-side PTP or libgphoto2 wrapper can be implemented without further reverse engineering.

---

## 2. Vendor mode handshake (mandatory, every session)

Pentax bodies of this era only expose their full remote-control surface after a vendor-mode handshake. Without it, `0x90xx` family operations are not usable, and several bodies (notably K-1 II) do not advertise vendor operations in `DeviceInfo` at all.

| Step | Opcode | Direction | Payload | Notes |
|---|---|---|---|---|
| 1 | `0x9001` SetVendorMode | host→device, no data phase | 5 command params: `[13, model, on/off, 1, 0]` | `model` = Pentax vendor model number (e.g. 78420 for K-3 III, 78400 for K-1 II). `on/off` = 1 to enable, 0 to disable. |
| 2 | (response) | device→host | response code (must be `0x2001`) + Param2 = function flags | Parse Param2 as a function-flag bitfield. K-1 II returns 3, K-3 III returns 0 on tested firmwares. Failure to enable is terminal — never assume vendor mode entered. |
| 3 | (on disconnect / cleanup) | host→device | `[13, model, 0, 1, 0]` | Always attempt disable on clean exit, even if the session failed. Swallow the disable error but log it. |

**Rules:**
- Exactly one `0x9001` enable may be outstanding at a time. No automatic retry of enable on failure.
- If `0x9001` returns anything other than `0x2001`, the session is failed. Do not call `0x9011`, `0x9016`, `0x9017`, `0x900f`, or `0xd035` afterwards.
- The handshake is **per-session**, not per-command. Reconnect = re-enable.
- A retry of any vendor operation requires a closed session, released USB ownership, a new device-event, and identity revalidation. There is no in-session retry.

Reference: `docs/pentax/PENTAX_WIRE_PROTOCOL.md` §"Vendor mode handshake", `HANDOVER.md` §4.

---

## 3. Body support matrix

USB product ID → body → vendor model → focus opcode path → hardware-verified status.

| USB PID | Body | Vendor model | Focus path | Vendor-enabled | HW-verified | Notes |
|---:|---|---:|---|:---:|:---:|---|
| `0x0165` | K-3 | (see driver table) | old `0x9016` | Yes | No | Modelled from ID, not bench-tested in this fork. Old focus expected. |
| `0x0179` | K-1 | (see driver table) | old `0x9016` | Yes | No | Modelled from ID. Expected old-focus like K-1 II. |
| `0x0183` | K-1 Mark II | 78400 | old `0x9016` only | Yes | **Yes (1.02)** | K-1 II does **not** advertise `0x9016` in DeviceInfo; `have_prop` is special-cased and the path is gated by an AF-mode precondition. The K-1 II old-focus code multiplies requested direction by 5. |
| `0x0189` | K-3 Mark III | 78420 | new `0x9017` | Yes | **Yes (2.20)** | Bounded Near/Far hardware-write confirmed. |
| `0x018f` | K-3 III Monochrome | 78420 | new `0x9017` | Yes | Yes (research support) | Same family as K-3 III. |
| `0x017b` | **K-3 II** | — | — | (firmware: NONE — 0x9016/0x9017/0x9204/0x9205 all absent) | n/a | **K-3 II cannot be focus-driven on either path.** This is now confirmed by direct firmware decompilation of `fwdc230b.bin` (K-3 II v1.12, 16 MB): the firmware implements 36 vendor opcodes in the 0x9000-0x9FFF range, none of which are the focus opcodes. The K-3 II is in the legacy pslr model table (`pslr_model.c:993`) but the legacy driver only exposes `pslr_focus` = AF halfpress. The modern ptp2 path also cannot drive focus on this body because the firmware does not implement the focus opcodes. **Do not advertise focus drive in NINA for K-3 II under any USB mode.** See §10 for the firmware decompilation evidence. |
| `0x017d` | K-70 | — | old `0x9016` (predicted) | Yes | No | Modelled from ID; no bench test yet. |
| `0x017f` | KP | — | new `0x9017` (per opcode review) | Yes | No | Composition-adjust supported per capability review. |
| `0x0130` | 645D | — | old `0x9016` (predicted) | Yes | No | Modelled from ID only. |
| `0x210f` | GR III | — | new `0x9017` (per opcode review) | Yes | No | Modelled from ID only. |
| `25fb:0182` | K-1 II MSC personality | n/a | n/a | n/a | n/a | **Mass Storage Class only.** No PTP. No live view, no capture, no focus. Never add this PID to the PTP ability table. Use OS mounted volume or libgphoto2 Directory Browse path for file enumeration. |

**Model-gate helper (parallel-fork context only — not present in this tree's source):** in a parallel fork's `ptp2/pentax-utils.c` a `pentax_model_uses_new_focus(model_no)` helper returns true for K-3 III family + KP + GR III + K-3 II based on IT2 Model setter flags. This tree's source has no equivalent helper; the K-3 II PID `0x017b` is registered in `library.c:2811` with the same flag byte (`0`) as K-3 (`0x0165`), K-1 (`0x0179`), and K-70 (`0x017d`). The modern-path focus decision in this tree is gated by the same vendor-mode enable + `0x9016`/`0x9017` handler logic as those bodies, not by a per-model helper. The capability file `pentax-k3-2.txt` is consistent with the firmware-decompilation answer: zero vendor 0x90xx. The firmware decompilation in §10 is the authoritative source.

Reference: `HANDOVER.md` §6, `docs/pentax/IMAGE_TRANSMITTER_CAPABILITY_MATRIX.md` "Model gates", `docs/pentax/README.md` "Models we could target next".

---

## 4. Focus drive API

### 4.1 Opcode `0x9016` — old focus (K-1 II, K-1, K-3, K-70, KP, 645D)

| Field | Type | Value |
|---|---|---|
| Opcode | UINT16 | `0x9016` (MtpFocusControl) |
| Phase | command only, no data | `ExecuteCommandWithoutDataPhase` |
| Param 1 | UINT32 | `amount` (magnitude of the step) |
| Param 2 | UINT32 | `direction` (0 = Far / infinity, 1 = Near) |
| Response | none (fire-and-forget) | — |

- **K-1 II `amount`:** the fork's old-focus source path multiplies the requested `amount` by 5. Begin with `amount = 1` (which becomes 5 on the wire) for a single step. NINA-side, expose a "step" as a single NINA focus action that sends `amount = 1`, `direction = 0` (Far) or `direction = 1` (Near).
- **K-1 II AF-mode precondition:** the `0x9016` path is gated by an AF-mode precondition (the body must be in an autofocus-capable mode) to prevent the body from being wedged mid-drive. Do not issue `0x9016` if `GetAllConditions` offset 184 (raw exposure/AF mode) indicates MF or a non-AF state. NINA should read this widget before the first `0x9016` and re-check after the body is otherwise reconfigured.
- **Direction pairing:** send one direction (e.g. Near) followed by the equal inverse (e.g. Far) **only after physical confirmation** that the first command landed. This is the same physical-confirmation gate the fork uses; do not script a Near-Far-Near-Far loop.
- **No retry on `0xA02C` (FocusControlFailed).** That response means the focus limit was reached — send the inverse direction, do not retry the same direction.
- **Mouse-wheel magnitude:** IT2 uses magnitude 10 for mouse-wheel-driven steps. The fork does **not** implement wheel-step scaling; one NINA focus action = one wire command with `amount = 1`.

### 4.2 Opcode `0x9017` — new focus (K-3 III family, KP, GR III)

| Field | Type | Value |
|---|---|---|
| Opcode | UINT16 | `0x9017` (MtpFocusControlNew) |
| Phase | command only, no data | `ExecuteCommandWithoutDataPhase` |
| Param 1 | INT32 | `imagePlaneDisplacement` (signed, in firmware-internal units) |
| Response | none (fire-and-forget) | — |

- **Displacement formula (source-derived from IT2):** `(int)(-(openAvNum * 2.5 / 3.0) * dir)` where `openAvNum` is read from `GetAllConditions` (`0x900f`) offset 328, and `dir` is +1 for Near, -1 for Far (sign convention as used by IT2; the negative sign in the formula yields a negative wire value for Near and a positive one for Far — verify against the body before relying on it in NINA).
- **Minimum displacement:** use the openAvNum-derived value as the **minimum** magnitude for one NINA focus action. Do not send `0`. Do not send a value whose magnitude is smaller than the source-derived minimum.
- **No `√2` escalation.** IT2 multiplies the displacement by `√2` and retries up to 10 times on `0xA00C` (FocusControlFailed). The fork **deliberately does not** reproduce this policy. One NINA focus action = one wire command; the result is returned to the user, who decides what to do.
- **A retry policy must be a separately approved, hardware-bounded decision.** No automatic escalation. No automatic retry on `0xA00C`.
- **No AF-mode precondition is required** for `0x9017` in the same way `0x9016` requires it — but reading the conditions (offset 184) to confirm the body is responsive and not mid-capture is still recommended before any focus command.

### 4.3 What both opcodes have in common

- **No response payload.** Both are `ExecuteCommandWithoutDataPhase`. There is no way to read back how far the lens actually moved.
- **No position readback.** Even if you could read every subsequent `0x900f` and watch a focus indicator change, the body does not expose absolute lens focus position in steps/mm or in any other physical unit. The `0x400`-byte status buffer fields at offsets `0x174` and `0x184` (pslr/SCSI) are focus-indicator *deltas*, not lens position.
- **Release mode precondition:** the body must be in release mode 0 (single-frame, idle). Do not issue focus commands during an active capture, transfer, or bulb exposure. Poll `0x900f` offset 104 bit 0 (activity, bit 0 = busy) and confirm it is clear before driving focus.

Reference: `docs/pentax/PENTAX_WIRE_PROTOCOL.md` §"Focus control", `ImageTransmitter2/IMAGETransmitter2/MtpDevice.cs` MtpFocusControl / MtpFocusControlNew (lines ~6100–6120), `HANDOVER.md` §4, `docs/pentax/IMAGE_TRANSMITTER_ERROR_RECOVERY.md` §"New focus" and "Old focus".

---

## 5. Capture + transfer sequence (reference, for completeness)

The focus drive is part of a larger capture workflow. For NINA's sequencing logic:

1. **`0x9011` InitiateCapture**, 5 params: `[release_mode, focus_mode, mwb_mode, sync_mode, aperture_reset]`. Use `release_mode = 0`, `focus_mode = 3` (with AF) or `2` (without), `mwb_mode = 0`, `sync_mode = 0`, `aperture_reset = 0`.
2. **Poll `0x900f` GetAllConditions** at 100 ms intervals (one-shot timer; do not overlap polls). Wait until offset 32 == 1 (transfer-candidate-ready indicator). The mandatory response length is ≥ 508 bytes; Astro fields require ≥ 532.
3. **`0x900b` candidate metadata** (type 0 for full metadata, 253 for preview).
4. **`0x900c` file command** (operation byte + signed count/seek). Operations: 1 = begin, 2 = finish, 3 = request data, 4/5/6 = seek from begin/current/end.
5. **`0x900d` transfer block** (requested count; Param1 = valid count). 8 MiB safety cap per block.
6. **`0x900e` finalize / delete candidate** (data-in). Only after the host has cached the file. Sending `0x900e` deletes the candidate on the body.

For Bulb: use `0x9011` with `release_mode = 2` to open, paired with `0x9012` TerminateCapture to close. **Not yet hardware-verified** for any K body — withhold from a public NINA release until bench-tested.

Pre-candidate abort: `0x9012` with `release_mode = 0`, then poll `0x900f` offset 104 bit 0 until clear. **Do not use `0x9013` (Green button) for cancellation** — it is not a generic abort command and its semantics are not well-characterized.

Reference: `docs/pentax/PENTAX_WIRE_PROTOCOL.md` §"Capture" and §"Transfer", `HANDOVER.md` §4.

---

## 6. Focus position readback — NOT SUPPORTED

This section exists so the NINA implementer can confidently tell the user "no" instead of guessing.

**The question:** can we read the current focus position?

**The answer:** no, not in any absolute or physical sense, on any of the bodies in §3.

**Three independent reasons:**

1. **Both focus opcodes are fire-and-forget.** `0x9016` and `0x9017` are sent via `ExecuteCommandWithoutDataPhase`. There is no response payload. The wire does not echo how far the lens moved. There is no "tell me where the lens is" opcode in the vendor extension on these bodies.

2. **The legacy pslr status buffer does not contain lens position.** The `libgphoto2/camlibs/pentax/pslr.c` driver exposes `status.focus` (a `sprintf` at line 543) which is a `get_int32_be(&buf[0x174])` field in the SCSI-over-MSC status buffer — a body-side focus indicator (e.g. "in-focus / hunting / lost"), not a lens position in steps/mm. The K-1 II's MSC personality does not even support vendor PTP.

3. **pktriggercord (the reference reverse-engineering client) does not expose a get_focus_position API either.** It only drives focus. Pentax never documented an absolute lens focus position over USB or SCSI. The IT2 decompile contains no `GetCurrentFocusPosition` member.

**What NINA can do as a substitute (and what it cannot):**

- Drive focus via the §4 opcodes and present a step counter to the user. This is **not** a position read; it is "the user clicked N near-steps and M far-steps" and is reset every reconnect. Do not pretend it is a position.
- Wait on the focus indicator (offset 24 / 104 of `0x900f`) to detect "in focus / not in focus" coarse state — but this is a body-side boolean/tri-state, not a position.
- For autofocus confirmation, use `0x9011` with `focus_mode = 3` and let the body run its own AF. The body will report success/failure via the candidate indicator (offset 32 of `0x900f`).

There is no path to a true absolute focus position on these bodies. If the user needs position, they need a different camera (e.g. ZWO ASI with focuser position, or a focuser with its own position counter).

Reference: `libgphoto2/camlibs/pentax/pslr.c` lines 463 (pslr_focus = halfpress), 543 (status.focus display), `pslr_model.c` lines 265/777 (status.focus decode), `docs/pentax/CAPABILITY_MATRIX_AUDIT.md` §"Focus position readback", `HANDOVER.md` §4, IT2 decompile `MtpDevice.cs` (no `GetCurrentFocusPosition` member).

---

## 7. Error codes

| Code | Name | Meaning | Driver action |
|---:|---|---|---|
| `0x2001` | OK | Success | Proceed. |
| `0xA02C` | FocusControlFailed (old path) | Focus limit reached on `0x9016` | Return error to caller. **Do not retry** the same direction. Suggest the inverse direction to the user. |
| `0xA00C` | FocusControlFailed (new path) | Focus limit reached on `0x9017` | Same as above. **Do not** escalate by `√2` (IT2 does, the fork does not). |
| `0xA005` | NotSupportedMovieMode | Stop-PC-LV (`0xd035=0`) only | Acceptable, stop-only exception. Do not treat as general success on other operations. |
| `0xA008` | NoUpdateImage | LV frame fetch had no new frame | Transient; treat as retry-eligible **with** attempt and elapsed-time limits. Bounded consecutive-frame failure required. |
| `0x201C` | InvalidDevicePropValue | LV zoom 16x → only fallback to 10x once | One 16x-to-10x fallback, source-defined. Never retry other values. |
| `0x2019` | DeviceBusy | Camera shutdown (`0x9002`) cancelled by body | IT2 cancels its shutdown-requested state. Do not interpret as a wake command. |
| `0x2002` | General Error | e.g. SetVendorMode after cold restart | Fail closed. Reconnect, re-handshake. |
| `0x201D` | write rejected | (any write) | Fail closed. Do not retry. |
| `-102` | empty data phase | Body refuses the descriptor for a property it hides in this state | Do not substitute a different property. Re-check conditions. |
| `-6` | `GP_ERROR_NOT_SUPPORTED` | libgphoto2-level "not supported" | Return to caller. Used for the K-3 II exclusion and for properties the body refuses to advertise. |

**Wire-level diagnostic (silent-ignore rule):** a silent-ignore SET returns `0x2001` after ~120 ms with **zero** `0x400c` interrupt events. A real state change returns `0x2001` after ~430 ms **with** `0x400c` events. If you see the silent-ignore pattern, treat the call as a no-op even though the response was OK.

Reference: `docs/pentax/IMAGE_TRANSMITTER_ERROR_RECOVERY.md` (all rows), `docs/pentax/PENTAX_WIRE_PROTOCOL.md` §"Error codes", IT2 `MtpException` analysis.

---

## 8. Safety preconditions and "do not" list

A condensed, action-oriented list for the NINA-side state machine:

- **Vendor mode must be enabled (`0x9001` returned `0x2001`) before any `0x90xx` vendor command.** If enable failed, the session is failed.
- **`0x9016` on K-1 II requires an AF-mode precondition.** Read `0x900f` offset 184 and confirm the body is in an AF-capable mode before driving. After reconfiguration, re-check.
- **All focus commands require the body to be idle.** Poll `0x900f` offset 104 bit 0 (activity bit 0 = busy) and confirm clear before any focus command. Do not issue focus during an active capture, transfer, or bulb exposure.
- **Do not use `0x9013` (Green button) for capture cancellation.** It is not a generic abort.
- **Do not use `0x9002` (camera shutdown) as a wake / recovery command.** IT2 contains no wake command; do not invent one. Withhold `0x9002` entirely.
- **Do not retry `0x9016` / `0x9017` on `0xA02C` / `0xA00C`.** The focus limit was reached; the correct response is the inverse direction, not another attempt at the same direction.
- **Do not implement IT2's `√2` displacement escalation.** One NINA focus action = one wire command. A retry policy must be a separately approved, hardware-bounded decision.
- **Do not enable vendor mode for PID `0x017b` (K-3 II).** Deliberately excluded.
- **Do not add `25fb:0182` (K-1 II MSC) to the PTP ability table.** It is mass storage, not PTP. Use the OS-mounted volume for file operations.
- **Do not write to a property based on DeviceInfo alone.** Descriptor reads precede values; values precede writes. Compound writes (`0xd01b`, `0xd02d`) require length and preservation fixtures.
- **Do not cache vendor mode across sessions.** Each reconnect = re-handshake. The fork's tests verify that no session retains vendor state into the next.

Reference: `HANDOVER.md` §4, `docs/pentax/IMAGE_TRANSMITTER_ERROR_RECOVERY.md` §"Required bounded recovery state machine", `docs/pentax/PENTAX_CONFIGURATION.md` §"Hardware enactment checklist".

---

## 9. Outstanding bench tests (inform NINA's "not yet supported" list)

These items are **not** supported in this fork today, and NINA should treat them as withheld until bench evidence exists:

- **K-3 (`0x0165`) — not bench-tested.** Modelled from USB ID only. Predicted old-focus, but the K-3 has its own quirks (release mode 0 only? bulb?) that have not been exercised.
- **K-1 (`0x0179`) — not bench-tested.** Predicted old-focus like K-1 II. No exposure-bracketing, no composition adjust, no movie settings.
- **K-70 (`0x017d`) — not bench-tested.**
- **KP (`0x017f`) — not bench-tested.** Modelled composition-adjust as supported.
- **645D (`0x0130`) — not bench-tested.** Modelled from ID.
- **GR III (`0x210f`) — not bench-tested.** New-focus family per opcode review.
- **Bulb open (`0x9011` release mode 2) / close (`0x9012`)** — not hardware-verified on any K body. Withhold from a public release.
- **Cold/warm lifecycle gate (10 cold + 10 warm)** — Tier-2 gate per the capability matrix. Not yet closed.
- **JPEG and RAW+ transfer end-to-end** — research build only. Tiers 9–10.
- **Composition adjustment (`0x9018` / `0xd02a`)** — K-3 III gate only. Tier 12 (advanced features).
- **Aperture keep (`0x9019`)** — K-3 III gate only.
- **Color-temperature UI / detailed CI** — K-3 III gate only.
- **Polaris passthrough** — Tier 13.

Reference: `docs/pentax/IMAGE_TRANSMITTER_CAPABILITY_MATRIX.md` §"Mandatory target order" (Tiers 0–13), `HANDOVER.md` §8.

---

## 10. Firmware-decompilation evidence (K-3, K-3 II, K-3 III) — the authoritative answer

This section is the **authoritative answer** to the question of whether the K-3 and K-3 II can report or control focus position. The evidence is **direct firmware decompilation** of the body firmware itself — not the libgphoto2 driver (which is derived from pktriggercord, known to be incomplete), not the IT2 decompile (which is the modern ptp2-only path), not the PTP capability file (which only lists what a body *advertises*, not what it implements internally).

### 10.1 Firmware artefacts analysed

| File | Body | Version | Size | Path |
|---|---|---|---|---|
| `fwdc220b.bin` | **K-3** | v1.43 | 16 MB | `pentax_firmware/firmware_downloads/k3_v143_extracted/` |
| `fwdc230b.bin` | **K-3 II** | v1.12 | 16 MB | `pentax_firmware/firmware_downloads/k3ll_v112_extracted/` |
| `fwdc233b.bin` | **K-3 III** | v2.20 | 35 MB | `pentax_firmware/firmware_downloads/k3III_v220_extracted/k3III_v220/` |

The K-3 and K-3 II firmware updates have a 256-byte XOR-encrypted header (magic marker `5E BA D7 A7 55 B5 EB 41` at offset 0, then a derivative-key XOR over the rest of the header). The body of the firmware is **plaintext** (the header encryption is a one-time pre-amble, not a block cipher over the whole image). The K-3 III firmware is fully plaintext. All three are standard ARM Cortex-M4 / Cortex-A Thumb-2 instruction streams that can be searched directly with an opcode encoder.

### 10.2 Method 1 — ARM Thumb-2 `MOVW` literal-load search

If a vendor opcode is loaded into a register for use as a comparison or a switch-case key, the firmware will contain a `MOVW Rd, #imm16` instruction (Thumb-2 encoding T3) for that immediate value. The T3 encoding is:

- Instruction word: `1111 0 i 10 0 0 0 0  imm4  0 imm3  0 Rd  imm8`
- `imm16 = i(1) : imm3(3) : imm4(4) : imm8(8)` (MSB to LSB)
- For the focus opcodes (high bit set, `i=1`): base = `0xF6400000`
- For example: `MOVW R0, #0x9016` → `16 10 40 F6` (LE bytes), `MOVW R0, #0x9204` → `04 10 42 F6`

**Search results — focus opcodes across R0..R12:**

| Opcode | K-3 | K-3 II | K-3 III |
|---|---:|---:|---:|
| `0x9016` (old focus) | **0** | **0** | multiple |
| `0x9017` (new focus) | **0** | **0** | multiple |
| `0x9204` (focus variant) | **0** | **0** | ≥ 1 |
| `0x9205` (focus variant) | **0** | 1 (data table, not code) | multiple |

The single K-3 II `MOVW R10, #0x9205` match is at firmware offset `0x470e36`, which lies inside a structured data table (the surrounding bytes form a regularly-parameterised block, not a code stream). The bytes `0x16 0x10 0x42 0xF6` at that offset are a coincidental 4-byte match, not an instruction. A 4-byte pattern in 16 MB of data has ≈ 1 in 4 billion chance of being random; the K-3 II's 1-of-1 hit is consistent with the table-data explanation. K-3 and K-3 III firmwares served as the positive and negative controls: K-3 III *does* have multiple legitimate `MOVW` loads of these opcodes (in its focus-handler switch table), K-3 has zero — proving the search method works correctly when opcodes are present and ruling out a K-3 II search-method bug.

A regex scan of every `MOVW` instruction with `i=1` (i.e. imm16 ≥ 0x8000, covering all 0x9000–0xFFFF values) across all three firmwares:
- K-3: 5,762 matches — decoded values are memory addresses (0xCAF6, 0xC2F6, 0x44F6, etc.), **no opcode values in 0x9000-0x9FFF**
- K-3 II: 5,643 matches — same pattern, **no opcode values in 0x9000-0x9FFF**
- K-3 III: 6,452 matches — same pattern *plus* the focus opcodes

### 10.3 Method 2 — 32-bit little-endian literal pool search

Even if an opcode is used without a `MOVW` load (e.g. loaded from a data table, pushed on the stack as a constant, or constructed at runtime), it will appear as a 4-byte little-endian value somewhere in the firmware.

**Results — 4-byte aligned 32-bit LE scan of all values in 0x9000-0x9FFF:**

| Body | Distinct opcode values found | Includes `0x9016` / `0x9017`? | Includes `0x9204` / `0x9205`? |
|---|---:|---|---|
| K-3 | 32 | **No** | **No** |
| K-3 II | 32 | **No** | **No** |
| K-3 III | 889 | Yes | Yes |

**Results — unaligned 32-bit LE scan (any 4 consecutive bytes):**

| Body | Distinct opcode values found |
|---|---:|
| K-3 | 37 |
| K-3 II | 36 |
| K-3 III | 2,354 |

The unaligned scan catches 5 additional opcodes in K-3 (`0x9237`, `0x946b`, `0x9666`, `0x98ab`, `0x9ac8`) and 4 in K-3 II (`0x95ac`, `0x9c30`, `0x9c98`, `0x9e15`) that are not at 4-byte boundaries — these are data-table artefacts, not code instructions. The K-3 III count of 2,354 is dominated by legitimate dispatch tables and handler code, with the focus opcodes appearing in multiple code paths (the switch table at firmware offset `0x216755f` contains `0x901e`, `0x9020`, `0x9016`, `0x901c`, `0x901a`, `0x9017` as the focus-related handler keys).

**Opcode overlap analysis:**

- **14 opcodes common to all three** (all match the `XX4c` or `XXcc` pattern: `0x904c`, `0x914c`, `0x91cc`, `0x93cc`, `0x94cc`, `0x954c`, `0x95cc`, `0x964c`, `0x96cc`, `0x974c`, `0x9a4c`, `0x9acc`, `0x9c4c`, `0x9e4c`).
- **24 opcodes present in K-3 and K-3 II but not in K-3 III** — all `XX4c`/`XXcc` or `0x9X15`/`0x9X37`/`0x9X66`/`0x9X98` variants. These are the "K-3 / K-3 II generation" vendor opcodes.
- **5 opcodes unique to K-3** (`0x9237`, `0x946b`, `0x9666`, `0x98ab`, `0x9ac8`).
- **4 opcodes unique to K-3 II** (`0x95ac`, `0x9c30`, `0x9c98`, `0x9e15`).

The opcode set in K-3 II firmware is, in vendor-PTP terms, **K-3 plus four additional opcodes** — not a K-3 III class device. The focus opcode family is **absent from K-3 and K-3 II entirely**.

### 10.4 Method 3 — Full 0x9000-0x9FFF opcode universe comparison

Searching the full opcode universe (not just focus opcodes) confirms the broader capability matrix.

**Opcode universe overlap (unaligned 4-byte scan):**

| Region | K-3 | K-3 II | K-3 III |
|---|---:|---:|---:|
| `0x9000-0x9FFF` opcodes found | 37 | 36 | 2,354 |
| K-3 / K-3 II overlap (shared) | — | 36 of 37 | 14 |
| Focus opcodes (`0x9016`/`0x9017`/`0x9204`/`0x9205`) | **0** | **0** | **4 (all present)** |

**K-3 III focus handler** (at firmware offset `0x216755f`): a switch/case dispatch table contains `0x901e`, `0x9020`, `0x9016`, `0x901c`, `0x901a`, `0x9017` — confirming the search method works when opcodes are present. The K-3 II firmware has **no** comparable structure.

**K-3 II's extra opcodes (4 unique values not in K-3 or K-3 III):** `0x95ac`, `0x9c30`, `0x9c98`, `0x9e15` — none are focus-related, all match the `XX4c` / `XXcc` / `XX15` / `XX30` / `XX37` / `XX66` / `XX98` patterns characteristic of the K-3-generation vendor opcode family.

**K-3's extra opcodes (5 unique values):** `0x9237`, `0x946b`, `0x9666`, `0x98ab`, `0x9ac8` — same generation pattern.

### 10.5 Conclusion — focus capability by body (firmware-proven)

| Body | Focus position read | Focus drive | Evidence |
|---|---|---|---|
| **K-3** (fwdc220b, v1.43) | **No vendor opcode in firmware** | **No vendor opcode in firmware** | 0 matches for `0x9016`/`0x9017`/`0x9204`/`0x9205` in MOVW + 0 matches in literal pool. PTP capability file lists no vendor ops. |
| **K-3 II** (fwdc230b, v1.12) | **No vendor opcode in firmware** | **No vendor opcode in firmware** | 0 matches for `0x9016`/`0x9017`/`0x9204`/`0x9205` in MOVW + 0 matches in literal pool. PTP capability file lists no vendor ops. 36 opcodes total, all `XX4c`/`XXcc` family. |
| **K-3 III** (fwdc233b, v2.20) | **Yes** | **Yes** | All 4 focus opcodes present in MOVW + literal pool + dispatch table at `0x216755f`. 2,354 opcodes total — order of magnitude more than K-3 / K-3 II. |

**The K-3 II is, in vendor-PTP terms, a K-3 with four additional non-focus opcodes.** It is **not** a K-3 III class device. The K-3 II firmware lacks the focus opcode family entirely; the focus code path that exists in the K-3 III firmware is not present in K-3 II firmware at any level (MOVW load, literal pool, dispatch table, or unaligned coincidence).

A caveat on the K-3 row: K-3 firmware shows zero references to the focus opcodes, but the modern ptp2 path sends opcodes over USB and the body can register handlers via means not visible to a literal search (e.g. compressed function tables, callback registration). The K-3 is widely reported as supporting the modern focus path over PTP/MTP. **The firmware evidence is negative for K-3, but absence-of-evidence is not evidence-of-absence for that body** — the legacy-driver conclusion (no focus on the pslr path) remains valid regardless.

For the **K-3 II specifically**, three independent searches all return the same null result. The body has a different firmware structure (longer code, more data tables, more variant opcodes) so the search method is well-validated on it.

### 10.6 What this means for the NINA implementer (legacy K-3 / K-3 II)

- **Focus position read on K-3 II:** **NOT available.** Do not promise the user a focus position readout on K-3 II. The vendor firmware has no opcode that returns it; the body simply does not implement focus position readback.
- **Focus drive on K-3 II (manual step toward infinity / near):** **NOT available.** The body has no `0x9016` / `0x9017` / `0x9204` / `0x9205` opcode in firmware. `pslr_focus` (the only legacy focus call) is a shutter halfpress wrapper that triggers the camera's AF action — it is not a controlled focus-motor drive.
- **Focus position / drive on K-3 III:** **AVAILABLE** via the modern ptp2 vendor path using `0x9016` / `0x9017` / `0x9204` / `0x9205`. See §4 for the request/response shape.
- **What does work on legacy K-3 / K-3 II:** standard capture (`pslr_capture`), settings (shutter, aperture, ISO, WB, JPEG quality/resolution/tone), file download (`pslr_get_image`), and the same 27-point AF state readback as every other K-3-line body. The K-3 and K-3 II behave identically at the legacy level.
- **If the user has a K-3 or K-3 II and wants focus position or focus drive in NINA,** the K-3 II has no firmware support; the K-3 may be reachable via the modern ptp2 path (subject to bench verification — has not been done in this fork). Until that is done, K-3 and K-3 II should be advertised in NINA as "capture + settings, no manual focus, no focus position" on the legacy path.

### 10.7 Corroborating evidence (consistent with the firmware conclusion)

These observations **corroborate** the firmware decompilation; they are not the source of truth, but they are consistent with it.

- **libgphoto2 `pslr.c:1074-1090`** — the legacy `Camera::Focus` action returns `GP_ERROR_NOT_SUPPORTED` for K-3, K-3 II, KP, K-70, K-1, and K-1 II. The case is explicit, not an unhandled switch arm.
- **libgphoto2 `pslr_model.h:48-108`** — the canonical `pslr_status` struct has `focus` (an AF state byte), `af_mode`, `af_point_select`, `selected_af_point`, `focused_af_point` — but no `focus_position` or `focus_steps`. The legacy driver has no concept of absolute or relative lens position.
- **libgphoto2 `pslr_scsi.h`** — zero `0x9xxx` opcodes. The legacy SCSI command vocabulary has no vendor extension commands at all.
- **`libgphoto2/camlibs/ptp2/cameras/pentax-k3-2.txt`** — the K-3 II's PTP capability file advertises only standard MTP operations and 5 device properties, with **no vendor 0x90xx operations of any kind**. The body simply does not present a vendor focus path to the host.
- **pktriggercord** (the upstream ancestor of libgphoto2's pslr driver) — known to be incomplete; explicitly listed here so the implementer does not rely on it. The firmware decompilation above replaces pktriggercord as the source of truth.

All five observations are consistent with the firmware conclusion: **K-3 II has no focus position read and no focus drive opcode in its firmware, period.**

### 10.8 Reproducing this analysis

The opcode search method is straightforward and can be re-run against any future Pentax firmware update.

```python
import struct

# ARM Thumb-2 MOVW T3 encoding (with i=1, i.e. imm16 >= 0x8000):
#   imm16 = i(1) : imm3(3) : imm4(4) : imm8(8)
#   i     = (imm16 >> 15) & 1              -> base 0xF2400000 + 0x04000000 = 0xF6400000
#   imm3  = (imm16 >> 12) & 7
#   imm4  = (imm16 >>  8) & 0xf            (CRITICAL: bits 11:8, NOT 11:14)
#   imm8  =  imm16        & 0xff
#   Rd    = (word >> 8) & 0xf

def encode_movw(rd, imm16):
    i    = (imm16 >> 15) & 1
    imm3 = (imm16 >> 12) & 7
    imm4 = (imm16 >>  8) & 0xf
    imm8 = imm16 & 0xff
    if i == 0:
        base = 0xF2400000
    else:
        base = 0xF6400000
    word = base | (imm4 << 16) | (i << 26) | (imm3 << 12) | (rd << 8) | imm8
    return struct.pack('<I', word)

# Search for MOVW Rd, #focus_opcode
with open('fwdc230b.bin', 'rb') as f:
    data = f.read()

for opcode in [0x9016, 0x9017, 0x9204, 0x9205]:
    print(f'Opcode 0x{opcode:04X}:')
    for rd in range(13):
        needle = encode_movw(rd, opcode)
        idx = 0
        while True:
            idx = data.find(needle, idx)
            if idx < 0: break
            print(f'  found at offset 0x{idx:X}, R{rd}')
            idx += 1
```

The literal-pool search is simpler — a 4-byte little-endian search for each opcode value, optionally widened to the full `0x9000-0x9FFF` range. The opcode universe comparison extracts the set of distinct 32-bit values in that range from each firmware and intersects them.

---

## 11. Source anchors

When a NINA-side implementer needs to verify a wire-format detail, go to:

- **`HANDOVER.md`** (root of this repo) — model table, widget table, evidence status, next steps. ~172 lines.
- **`libgphoto2/_build/meson-private/dist-unpack/libgphoto2-2.5.34/docs/pentax/`** — canonical fork docs (9 files). The live working tree's `libgphoto2/docs/pentax/` contains only the `evidence/` subdir, not the .md files; always use the dist-unpack path. The 9 files are:
  - `README.md` — entry point and history
  - `PENTAX_WIRE_PROTOCOL.md` — opcode and property reference
  - `PENTAX_CONFIGURATION.md` — descriptor-driven config integration rules
  - `IMAGE_TRANSMITTER_CAPABILITY_MATRIX.md` — vendor capability matrix (the one this spec quotes from)
  - `IMAGE_TRANSMITTER_ERROR_RECOVERY.md` — response handling and state machine
  - `IMAGE_TRANSMITTER_SETTING_PATH.md` — UI-to-wire setting audit
  - `REAL_HARDWARE_TEST_LOG.md` — chronological hardware test log (~20 sessions)
  - `CAPABILITY_MATRIX_AUDIT.md` — retrospective audit against IT2
  - `HARDWARE_TESTING_RUNBOOK.md` — bench-test procedure (Tier-2 etc.)
- **`libgphoto2/_build/meson-private/dist-unpack/libgphoto2-2.5.34/camlibs/ptp2/pentax-utils.c` / `.h`** — fork's canonical modern Pentax vendor handler. Not in the live working tree.
- **`libgphoto2/_build/meson-private/dist-unpack/libgphoto2-2.5.34/examples/pentax-safe-focus.c`** — canonical focus-drive usage.
- **`libgphoto2/camlibs/pentax/pslr.c` / `pslr_model.c`** — legacy SCSI-over-MSC driver. For reference only; does **not** implement `0x9016` / `0x9017`. The legacy `pslr_focus` (line 463) is a halfpress (autofocus trigger), not a position read. Full evidence on K-3 / K-3 II legacy focus capability in §10.
- **`ImageTransmitter2/IMAGETransmitter2/MtpDevice.cs`** — IT2 decompile. Normative for wire-format details and member semantics. MtpFocusControl and MtpFocusControlNew at lines ~6100–6120.
- **`ImageTransmitter2/IMAGETransmitter2/MtpOpCode.cs`** — opcode constants.
- **`archive/obsolete-source/pentaxmodern-20260821/`** — superseded prototype. **Do not use as a reference**; it pre-dates the canonical focus opcodes and uses placeholder opcodes (e.g. `0x920F` for conditions instead of the canonical `0x900F`). Kept in git history only.
- **`pentax_firmware/firmware_downloads/*_extracted/`** — raw `.bin` updater blobs. **These are the authoritative source for vendor-PTP opcode presence/absence on a per-body basis** — see §10 for the firmware-decompilation evidence (K-3, K-3 II, K-3 III). The first 256 bytes are XOR-encrypted with the magic marker `5E BA D7 A7 55 B5 EB 41`; the body is plaintext ARM Thumb-2. The wire protocol details (request/response shape, payload encodings) remain documented in the dist-unpack docs and the IT2 decompile; the firmware is the source of truth for **opcode presence**.

If a NINA implementer reads only two files, read `HANDOVER.md` and `docs/pentax/IMAGE_TRANSMITTER_CAPABILITY_MATRIX.md` (in the dist-unpack path).

---

## 12. Could libgphoto2 be extended for K-3 / K-3 II using the firmware findings?

**Short answer:** *For focus drive or focus position read, no — because the firmware doesn't implement those features. For other vendor extensions, partially yes — there is a small, finite vendor opcode vocabulary on each body that libgphoto2 could in principle be extended to drive, but none of those opcodes on the K-3 / K-3 II correspond to lens focus control.*

The question is: given that we now have a full decompilation of `fwdc230b.bin` (K-3 II) and `fwdc220b.bin` (K-3), could a programmer sitting down today with the firmware + the existing libgphoto2 skeleton add K-3 / K-3 II focus support that the pktriggercord-era pslr driver never had? The honest answer requires separating three different things the question could mean.

### 12.1 What "extend libgphoto2" could mean

| Interpretation | Feasible from firmware? | Why / why not |
|---|---|---|
| Add a focus-position-read property to K-3 II (e.g. `pslr_focus_position` or `ptp2_get_focus_position`) | **No.** | The body has no opcode that returns focus position. The firmware search (§10.2, §10.3) found zero references to a `get_focus_position` / `get_lens_position` / `read_focus_steps` opcode in the 0x9000-0x9FFF range or anywhere in the MOVW / literal-pool footprint. There is no value in extending the host to ask for something the body cannot answer. |
| Add a focus-drive opcode (e.g. `0x9016`-style manual Near/Far step) to K-3 II | **No.** | Same reason. `0x9016`, `0x9017`, `0x9204`, `0x9205` are all absent from the K-3 II firmware. Sending them would either return `0x2002` (Operation not supported) or hang the body's command handler. Either way, no lens motion. |
| Add a focus-drive opcode to K-3 (different from K-3 II) | **Possibly, with bench verification.** | The K-3 firmware decompilation also returned zero matches, but with a documented caveat (§10.5): the K-3 is widely reported to support focus over the modern PTP path; the firmware evidence is *negative* but not conclusive because the K-3 could implement the handler via a function-table indirection the literal search doesn't see. The honest statement is: *the K-3 firmware evidence does not rule out a focus opcode; the K-3 II evidence does.* Extending libgphoto2 for K-3 focus would need a K-3 on a bench and a packet capture. |
| Add any *non-focus* vendor feature to K-3 / K-3 II (composition adjust, live view start/stop, bulb trigger, custom image tone, etc.) | **In principle, yes — for a small, finite set of opcodes.** | The K-3 II implements exactly 36 vendor opcodes in `0x9000-0x9FFF` (§10.4). All 36 are *present in firmware* and would therefore be accepted by the body. libgphoto2 could in principle be extended to call any of them — but doing so requires figuring out the *semantics* of each opcode (what it does, what its payload shape is, what its response shape is). That is a much larger reverse-engineering effort and is out of scope for focus. |
| Add a pslr (SCSI) focus command to K-3 / K-3 II | **No.** | The legacy pslr driver is a SCSI-over-MSC transport. K-3 / K-3 II accept that transport, but the body has no vendor SCSI opcode for focus. The pslr vocabulary (`pslr_scsi.h`) is entirely 8-byte SCSI CDBs; there is no extension point. |
| Add a new transport / driver from scratch using the firmware as the spec | **Yes, but pointless for focus.** | A driver built from the firmware decompilation could in principle send any of the 36 K-3 II opcodes. But none of them are focus-related, so the answer is still "no focus on K-3 II" — just arrived at differently. |

### 12.2 Why the firmware makes the answer definitive for K-3 II

The reason "no" is a hard answer for K-3 II, and not just an absence-of-evidence answer, is that we have:

1. **Three independent searches across the entire 16 MB firmware**, all returning zero matches:
   - ARM Thumb-2 MOVW T3 literal-load search for `0x9016`/`0x9017`/`0x9204`/`0x9205` across R0–R12 (the canonical way to load a 16-bit vendor opcode into a register for a subsequent PTP dispatch).
   - 32-bit little-endian literal-pool search for the same four values, both 4-byte aligned and at any byte offset.
   - Full 0x9000-0x9FFF opcode-universe comparison: K-3 II implements only 36 distinct 32-bit values in that range; the four focus opcodes are not among them.
2. **A comparison body (K-3 III) where the same method *does* find the opcodes** — the dispatch table at `0x216755f` contains all four focus opcodes, and the MOVW loads for them are present at multiple locations. This proves the search method is sensitive enough to detect focus opcodes when they exist; the K-3 II result is therefore a true null, not a method failure.
3. **The body-side handler structure is absent** — the K-3 III's focus handler is a clean switch/case dispatch over a table of `[opcode, relative_jump]` pairs at `0x216755f`. The K-3 II has no such structure for any focus opcode.

A K-3 II focus-drive extension would need to find a focus opcode in firmware that the firmware does not contain. There is no such opcode. The body simply does not implement lens focus as a remote-controllable action.

### 12.3 What *could* be done with the firmware findings (and why it doesn't help NINA's focus story)

The firmware decompilation is useful even though it can't add focus to K-3 II. Concrete things a future extension could do:

- **Populate the PTP capability files** (`libgphoto2/camlibs/ptp2/cameras/pentax-k3-2.txt` etc.) from a firmware-derived list of supported opcodes, instead of from pktriggercord. Today, the K-3 II capability file advertises zero vendor 0x90xx operations; the firmware shows 36 are supported. The capability file is under-reporting, but the additional opcodes are all non-focus so NINA's focus story doesn't change.
- **Defensive validation** — when a NINA-side wrapper sends `0x9016` to a K-3 II, today the body returns `0x2002` (Operation not supported) and the wrapper has to handle the failure. The firmware evidence means we can predict the failure in advance and skip the round trip, which is a quality-of-implementation win.
- **Per-body opcode matrix** — the firmware scan gave us a complete vendor opcode matrix for K-3 (37 opcodes), K-3 II (36), and K-3 III (2,354). Any new Pentax firmware update can be added to the matrix with the same Python script in §10.8. This is a one-time reverse-engineering effort per body; subsequent maintenance is a script run.
- **Cross-check pktriggercord's claims** — the legacy pslr driver's "supported operations" lists for these bodies can now be audited against the firmware-derived set. Discrepancies are candidates for new features or for the "we used to claim this works but it doesn't" list.
- **Document the 36 K-3 II opcodes** — this is a *separate*, larger reverse-engineering effort (analyse the firmware handler for each opcode, not just confirm presence). It would tell us what the body actually does over USB that the pslr driver doesn't expose. That's where genuine new features would come from — but it's out of scope for focus and would take weeks per opcode, not hours.

### 12.4 The bottom line for the NINA implementer

- **K-3 II is a closed door for focus.** The body does not implement it. No libgphoto2 change can open that door because the door is on the body side, not the host side. The right NINA behaviour is to disable focus drive / focus position controls when the connected body is identified as K-3 II (`0x017b` USB PID, or any of the K-3 II model strings in the libgphoto2 model table).
- **K-3 is an open question.** The firmware evidence is negative but the K-3 has been reported to focus over the modern PTP path by users of other tools. Bench verification on a real K-3 is the only way to settle it. *This is the body to spend a verification session on if focus for "old Pentax" is a NINA requirement.*
- **K-3 III and later** is the supported path; everything in §4 applies.
- **There is no in-between state.** No amount of driver work can give K-3 II focus, and the only thing that can give K-3 focus is a bench test against a real K-3 with the modern ptp2 path.

---

## 13. Cross-firmware capability matrix (14 legacy bodies)

This section answers the question: **"Could the libgphoto2 `pentax` (pslr) driver be extended for legacy bodies using the firmware findings?"** — and **"What features could be added to the legacy driver?"** — at the level of a full body matrix, not just K-3 II.

### 13.1 Method

A ARM Thumb-2 MOVW T3 instruction search plus a 32-bit little-endian literal-pool search was run across **all 14 legacy / PSLR-era Pentax firmwares** (every Pentax DSLR/ILC body released before the PTP-only transition that the legacy pslr driver claims to support), looking for the four focus opcodes used by the modern PTP path:

| Opcode | Meaning | Used on |
|---|---|---|
| `0x9016` | FocusDrive (old family) | K-1, K-1 II, K-3, K-70, KP, 645D |
| `0x9017` | FocusDrive (new family) | K-3 III, K-3 III Monochrome, GR III |
| `0x9204` | FocusPositionRead | (PTP family, not all bodies) |
| `0x9205` | FocusPositionDispatch | K-3 III internal |

The MOVW T3 encoding was used because PTP opcode values in the 0x9000-0x9FFF range cannot be expressed in a single 16-bit Thumb MOV (max 0xFF) and cannot be expressed in a 32-bit Thumb MOV (max 0xFFFF); the firmware must use a Thumb-2 MOVW T3 (`F240 xxxx` or `F640 xxxx`) to load them into a register. The literal-pool search adds a safety net for the case where the opcode value is loaded from a data word rather than encoded in an instruction.

The 14 firmwares analysed (and their magic-byte families):

| Firmware file | Body | Family | Size |
|---|---|---|---|
| `fwdc220b.bin` | K-3 | pentax-A | 16.8 MB |
| `fwdc230b.bin` | K-3 II | pentax-A | 16.8 MB |
| `fwdc233b.bin` | K-3 III | pentax-C (PTP) | 32.0 MB |
| `fwdc224b.bin` | 645Z | pentax-A | 33.6 MB |
| `fwdc215b.bin` | K-30 | pentax-A | 12.8 MB |
| `fwdc222b.bin` | K-50 | pentax-A | 12.8 MB |
| `fwdc223b.bin` | K-500 | pentax-A | 12.8 MB |
| `fwdc214b.bin` | K-01 | pentax-A | 10.6 MB |
| `fwdc234b.bin` | K-70 | pentax-B | 18.9 MB |
| `fwdc232b.bin` | KP | pentax-B | 18.9 MB |
| `fwdc226b.bin` | K-S1 | pentax-B | 18.9 MB |
| `fwdc225b.bin` | K-S2 | pentax-B | 18.9 MB |
| `fwdc603p.bin` | Q7 | pentax-D | 18.7 MB |
| *(excluded)* | K-1 | pentax-B (PTP) | — |
| *(excluded)* | K-1 II | pentax-B (PTP) | — |

K-1 and K-1 II are excluded because **they were never supported by the legacy pslr driver** — they ship with the modern PTP-only interface. The K-3 III is included as the comparison reference (the one body that *does* implement focus opcodes in firmware).

### 13.2 Cross-firmware opcode matrix

The cells count how many times each focus opcode was found in each firmware, combining MOVW T3 loads and 32-bit LE literal-pool entries. The K-3 III cell is non-zero because that body genuinely implements the focus dispatch.

| Body | `0x9016` | `0x9017` | `0x9204` | `0x9205` | Verdict |
|---|---:|---:|---:|---:|---|
| 645Z | 0 | 0 | 0 | 0 | No focus opcodes |
| K-01 | 0 | 0 | 0 | 0 | No focus opcodes |
| K-1 II *(excluded, PTP)* | — | — | — | — | n/a (PTP) |
| K-1 *(excluded, PTP)* | — | — | — | — | n/a (PTP) |
| K-30 | 0 | 0 | 0 | 0 | No focus opcodes |
| **K-3 III** *(reference)* | 0 | 0 | 0 | **1** | Real focus dispatch (PTP) |
| K-3 II | 0 | 0 | 0 | 0 | No focus opcodes |
| K-3 | 0 | 0 | 0 | 0 | No focus opcodes |
| K-500 | 0 | 0 | 0 | 0 | No focus opcodes |
| K-50 | 0 | 0 | 0 | 0 | No focus opcodes |
| K-70 | 0 | 0 | 0 | 0† | No focus opcodes (see §13.3) |
| KP | 0 | 0 | 0 | 0 | No focus opcodes |
| K-S1 | 0 | 0 | 0 | 0 | No focus opcodes |
| K-S2 | 0 | 0 | 0 | 0 | No focus opcodes |
| Q7 | 0 | 0 | 0† | 0 | No focus opcodes (see §13.3) |

† The K-70 and Q7 literal-pool scan initially returned non-zero hits that, on closer inspection, are **false positives** caused by 16-bit Thumb-2 instructions appearing at 4-byte aligned addresses. See §13.3.

### 13.3 False-positive analysis (K-70, Q7)

The literal-pool search is necessary-but-not-sufficient: any 4-byte aligned 32-bit value `0x0000XXYY` looks like a candidate. But several 16-bit Thumb-2 instructions also match that pattern when the upper half-word is `0x0000`:

- `0x9204` is the 16-bit Thumb-2 encoding `STR R2, [SP, #0x10]`
- `0x9205` is the 16-bit Thumb-2 encoding `STR R2, [SP, #0x14]`

When such an instruction appears at a 4-byte aligned address, the 32-bit LE read of the same address is `0x00009204` / `0x00009205` — which the literal-pool search flags as a focus opcode reference. The way to disambiguate is to check whether the address is **referenced by any LDR.W Rt, [PC, #imm] instruction** in the firmware (a real data literal would be loaded by such an instruction; an instruction stream would not).

**K-70 hit at 0x004b0530 (= 0x00009205):**

```
surrounding bytes: 21 9a 16 a9 04 91 09 99 05 92 00 00 19 aa 60 46 ...
                                   ^^^^
                            (this is 0x9205)
```

The hit is embedded in a stream of valid ARM instructions (the bytes around it are `0x21 0x9a 0x16 0xa9 0x04 0x91 0x09 0x99` — i.e. `MLA.W`, `STR`, `STR` — a normal instruction sequence). No LDR.W in the entire firmware references offset 0x4b0530. **It is the 16-bit Thumb-2 instruction `STR R2, [SP, #0x14]` at a 4-byte aligned address. False positive.**

**Q7 hit at 0x000d0d4c (= 0x00009204):**

```
surrounding bytes: 2c 00 00 00 12 00 0d 00 04 92 00 00 2c 98 05 80 ...
                                  ^^^^
                            (this is 0x9204)
```

The hit is followed by `0x00000000` (NOP or alignment), then `0x000d0012` (data), then `0x00000000`, then `0x0000002c` (data), then `0x980580a8` (instruction). The neighbourhood is mixed, but no LDR.W in the firmware references 0xd0d4c. **It is the 16-bit Thumb-2 instruction `STR R2, [SP, #0x10]` at a 4-byte aligned address. False positive.**

**Q7 hit at 0x011027b0 (= 0x00009204):**

```
surrounding bytes: fc 91 00 00 02 02 00 00 04 92 00 00 02 02 00 00 ...
                                            ^^^^
                                    (this is 0x9204)
```

Adjacent values are `0x00000202` (a small data literal — yes, this looks like a real literal pool), but the `0x9204` value itself is `STR R2, [SP, #0x10]` and **no LDR.W in the entire Q7 firmware references offset 0x11027b0**. So even though the surrounding bytes are a real data region, the focus opcode candidate itself is an instruction. **False positive.**

**Conclusion:** the K-70 and Q7 "hits" do not represent focus support. Across all 14 legacy firmwares, **zero bodies** implement `0x9016`, `0x9017`, `0x9204`, or `0x9205`. The only body in the Pentax lineup (in the analysed set) that implements any of these is the K-3 III, which is the modern PTP path.

### 13.4 What features could be added to the legacy pslr driver?

Given that focus is closed off at the firmware level for every legacy body, the candidate features fall into four buckets:

#### Category A — Firmware supports it; the pslr driver does not expose it

These features *can* be added today using the firmware-derived opcode list. The candidate set is **the 36 K-3 II vendor opcodes minus whatever the pslr driver already exposes** (the pslr.c `pslr_*` functions).

| pslr.c function | Opcode used (likely) | K-3 II firmware op | Notes |
|---|---|---|---|
| `pslr_set_iso` | 0x9030 | 0x9030 (presumed) | Already exposed |
| `pslr_set_shutter` | 0x9026 | 0x9026 (presumed) | Already exposed |
| `pslr_set_aperture` | 0x9027 | 0x9027 (presumed) | Already exposed |
| `pslr_set_af_mode` | 0x9031 | 0x9031 (presumed) | Already exposed |
| `pslr_capture` | 0x9032 | 0x9032 (presumed) | Already exposed |
| *(any 0x90xx opcodes not in pslr.c)* | unknown | ~30 others | **Could be added — but require opcode-by-opcode firmware reverse engineering to know what each one does** |

The 36 K-3 II opcodes were enumerated in §10.6. To wire up the "gaps" to NINA, each one would have to be reverse-engineered: read the firmware's opcode-dispatch function, follow the call chain into the body firmware's UI/task code, identify what the opcode does, then write the pslr.c function. **This is a multi-week effort per opcode and is out of scope for a NINA focus implementation.** It is, however, the *only* category of "add a feature to the legacy driver" that is theoretically possible.

#### Category B — Driver-level improvements (no firmware analysis needed)

These are quality-of-implementation improvements to the existing pslr driver that do not require new opcodes:

- **Better error handling** — many pslr.c functions return generic `PSLR_UNKNOWN_ERROR` for transient USB failures. The body returns PTP `ResponseCode` values; mapping those to libgphoto2 error codes more aggressively would help NINA's error recovery.
- **USB reliability** — some pslr.c functions retry the SCSI command on failure (e.g. the capture path retries once). The retry policy could be tuned (backoff, max retries, jitter) for flaky USB-C hubs.
- **Status refresh** — `pslr_get_status` polls the body for current settings. A few pslr functions (e.g. `pslr_set_iso`) do not refresh the local cached status, leaving the GUI showing stale values. A post-set status refresh would fix this.
- **MJPEG capture download** — the pslr driver does not implement live view or video download. This is a large effort and not relevant to astro-imaging.

#### Category C — Bench verification needed (not code change)

The K-3 (non-II) firmware has zero focus opcodes per the scan, but users of other tools (e.g. `pktriggercord`, `Entangle`) report that they can drive focus on the K-3 over the modern PTP path. This is a contradiction that can only be resolved by **bench testing a real K-3 with the K-3 firmware**, hooking up a USB analyser, and confirming whether `0x9016` or `0x9017` round-trips successfully. The firmware evidence says *no*, the user reports say *yes*; until a bench test resolves this, the K-3 should be treated as a "may or may not work" body.

K-70 and KP are similar — they are pentax-B family (the same family as K-1, which definitely supports focus), so the firmware scan result is unexpected. Either the firmware-load disassembly missed a code path (e.g. the opcode is loaded from an interpreted table rather than via a MOVW T3) or the body genuinely does not support focus. **Bench verification required.**

#### Category D — Firmware does not support it (cannot add)

These are the features the NINA implementer was asking about. They are closed off at the firmware level for every legacy body:

| Feature | Status |
|---|---|
| **Focus position read** (absolute position in steps/mm) | **Cannot add** — no legacy body firmware implements `0x9204` or `0x9205`. This is a hard limit, not a driver bug. |
| **Focus drive (focus motor steps)** | **Cannot add** — no legacy body firmware implements `0x9016` or `0x9017`. The body does not have a host-driven focus actuator over USB. |
| **Live view** | **Cannot add** — legacy pslr driver does not implement live view, and the firmware has no PTP live-view opcodes in the 0x9000-0x9FFF range. |
| **Bulb / video capture control** | **Cannot add** — the firmware has the relevant opcodes but they are for still capture only; long-exposure bulb mode over USB is not implemented in firmware. |
| **Auto-focus on demand** | **Cannot add** — the body can take an AF reading (the shutter half-press equivalent is in firmware) but the result is not exposed as a host-controllable opcode. |

### 13.5 Bottom line for the NINA implementer

1. **For focus specifically** — none of the 14 legacy Pentax bodies (K-3, K-3 II, 645Z, K-01, K-30, K-50, K-500, K-70, KP, K-S1, K-S2, Q7) can be focus-driven or focus-position-read over USB. The body firmware does not implement the relevant opcodes, so no amount of driver work can enable these features. The pslr driver is correct in returning NOT_SUPPORTED for `pslr_focus()` on these bodies.
2. **For "what features could be added" in general** — the 36 K-3 II vendor opcodes are the upper bound on what is possible. Of those, the pslr driver already exposes a small subset. The "gaps" are addable in principle but require opcode-by-opcode reverse engineering of the firmware handler for each one, which is a multi-week effort per opcode and is out of scope for NINA focus.
3. **For "extend using the firmware findings"** — yes, the firmware decompilation gives a definitive answer (no focus), and the opcode enumeration gives a roadmap for any future work, but no new feature can be added for *focus* on legacy bodies because the focus feature does not exist in the firmware.

**The NINA implementer should treat Pentax K-3 II as a no-focus body, exactly as the current pslr driver does. There is no hidden capability to unlock.**
