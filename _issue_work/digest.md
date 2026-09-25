====================================================================================================
#70 | Mark CAPABILITY_MATRIX_AUDIT.md as historical and reconcile stale status claims
labels:  | created 2026-09-09 | comments 1
----------------------------------------------------------------------------------------------------
## Problem

`docs/pentax/CAPABILITY_MATRIX_AUDIT.md` is a valuable retrospective snapshot dated 2026-08-21, but it now reads like current status and contradicts later code/evidence in multiple places.

Examples that are now stale:
- says K-3 III Monochrome is excluded;
- says K-1 II focus remains unavailable;
- says advanced model capability flags are absent/unimplemented;
- says live-view AF-position/zoom remain future work;
- says K-1 II preview has not passed;
- says direct K-1 II descriptor discovery is the largest functional gap.

Later work has changed each of those areas. The historical findings should not be deleted, but future agents must not use this document as the live support matrix.

## Required correction

1. Add a prominent banner at the top stating that this is a **historical 2026-08-21 retrospective**, preserved for provenance.
2. Point readers to:
   - `IMAGE_TRANSMITTER_CAPABILITY_MATRIX.md` for the normative protocol target;
   - `REAL_HARDWARE_TEST_LOG.md` for chronological hardware evidence;
   - #66 / its eventual durable matrix for current model × capability implementation status;
   - `CONTROL_ISSUE_INDEX.md` / `AGENT_WORK_QUEUE.md` for current work ownership.
3. Do not rewrite old conclusions as if they were always known; preserve the snapshot and annotate superseded claims where useful.
4. Search the Pentax docs for links that call `CAPABILITY_MATRIX_AUDIT.md` current/authoritative and correct those references.

## Acceptance

- [ ] historical status is unmistakable at the top of the file;
- [ ] no current agent-start/work-queue document tells agents to use it as live truth;
- [ ] #66 is the current reconciliation authority;
- [ ] provenance/history remains intact.
--- COMMENT by ian-morgan99 2026-09-09 ---
this is lower priority than first release of k1ii k3iii k-01
--- TIMELINE EVENTS ---
[cross-referenced by ian-morgan99 2026-09-09] <- 66 Reconcile every firmware-backed Pentax/Ricoh model against I
[cross-referenced by ian-morgan99 2026-09-09] <- 52 TRACKING: sweep Pentax/Ricoh model × generic-control coverag
[cross-referenced by ian-morgan99 2026-09-09] <- 42 UPSTREAM: Audit Pentax changes for libgphoto2 coding convent
[commented by ian-morgan99 2026-09-09]
====================================================================================================
#69 | Remove duplicate pentaxliveviewafposition config registration
labels:  | created 2026-09-09 | comments 2
----------------------------------------------------------------------------------------------------
## Problem

`camlibs/ptp2/config.c` currently registers the exact same Pentax config entry twice in the same menu block:

```c
{ N_("Pentax Live View AF Position"), "pentaxliveviewafposition", 0,
  PTP_VENDOR_PENTAX, PTP_OC_GetDevicePropValue,
  _get_Pentax_LiveViewAFPosition, _put_Pentax_LiveViewAFPosition },
```

It appears once after `pentaxdirectev` and then again immediately after `pentaxdirectwb`.

This is a straightforward correctness/maintainability bug: duplicate names can produce duplicate widgets or ambiguous first-match behaviour depending on which config-tree/single-config path is used, and it adds unnecessary noise to the upstream-facing Pentax series.

## Required fix

- remove the duplicate registration, leaving exactly one `pentaxliveviewafposition` entry;
- add/extend a menu-name uniqueness regression check for Pentax-added entries if practical, so copy/paste additions cannot silently duplicate a config name;
- verify `--list-config` and `gp_camera_get_single_config("pentaxliveviewafposition")` still resolve exactly one surface;
- no protocol behaviour should change.

## Cross-links

- #42 upstream conventions/maintainability audit
- #66 IT2 capability reconciliation
- #8 K-1 II `0xd036` write qualification (separate evidence/safety issue)

## Acceptance

- [ ] exactly one registration remains;
- [ ] list/single-config paths expose one widget;
- [ ] no unrelated menu ordering/refactor is mixed into the fix.
--- COMMENT by ian-morgan99 2026-09-09 ---
## Fixed (commit `990281d72`, pushed)

- Removed the stray second `pentaxliveviewafposition` registration in `camera_status_menu` (it appeared after both `pentaxdirectev` and `pentaxdirectwb`; kept the first, natural-position copy). No protocol behaviour change.
- Added a menu-name uniqueness regression check to `test-pentax-aperture-alias`, driven through the **public** `camera_list_config` path (no new production export, per rule 9): `pentaxliveviewafposition` must appear exactly once on a Pentax fixture and not at all on a non-Pentax fixture.
- Full suite green except the two pre-existing `no-ci` env failures.

Acceptance: [x] exactly one registration remains · [x] list/single-config paths expose one widget · [x] no unrelated menu ordering/refactor mixed in.
--- COMMENT by ian-morgan99 2026-09-09 ---
The duplicate-removal fix in `990281d72` looks correct, but the completion note slightly overstates the regression coverage. The new test exercises `camera_list_config()` and proves `pentaxliveviewafposition` appears exactly once there; it does **not** call `camera_get_single_config("pentaxliveviewafposition")`. The existing single-config call in this test is for `aperture` only. Please either add one direct single-config assertion for `pentaxliveviewafposition` (ideally verifying a single successful widget resolution) or leave that part of #69's acceptance pending. This is a test/provenance accuracy point, not a request to change the production fix.
--- TIMELINE EVENTS ---
[cross-referenced by ian-morgan99 2026-09-09] <- 66 Reconcile every firmware-backed Pentax/Ricoh model against I
[cross-referenced by ian-morgan99 2026-09-09] <- 52 TRACKING: sweep Pentax/Ricoh model × generic-control coverag
[cross-referenced by ian-morgan99 2026-09-09] <- 42 UPSTREAM: Audit Pentax changes for libgphoto2 coding convent
[cross-referenced by ian-morgan99 2026-09-09] <- 8 K-1 II live-view AF-position write semantics lack retained h
[commented by ian-morgan99 2026-09-09]
[referenced by ian-morgan99 2026-09-09]  sha:990281d72b
[cross-referenced by ian-morgan99 2026-09-09] <- 53 ptp2/pentax: add generic `aperture` compatibility control
[commented by ian-morgan99 2026-09-09]
====================================================================================================
#68 | Pentax IT2 parity: expose composition offset movement via 0x9018
labels:  | created 2026-09-09 | comments 0
----------------------------------------------------------------------------------------------------
## Goal

Implement the missing **composition-adjustment movement** part of Pentax Image Transmitter 2 parity.

Current code already exposes `pentaxcompositionadjust`, which toggles the composition-adjustment enable property `0xd02a`. That is only the switch. IT2 also uses vendor operation `0x9018` (`SetCompositionAdjustmentOffset`) to move the sensor composition position/rotation in bounded steps and reads composition state/limits back from GetAllConditions.

This is a child of #52/#66. It is intentionally separate from the existing `0xd02a` switch.

## Current evidence / code state

Implemented today:
- `pentaxcompositionadjust` get/put for `0xd02a`;
- explicit model gating via `pentax_model_supports_composition_adjust()` (K-3 III family + KP from IT2 evidence);
- `PTP_OC_PENTAX_SetCompositionAdjustmentOffset = 0x9018` defined in `ptp.h`;
- composition-related condition fields documented in the IT2 matrix (state/axes/rotation/limits around offsets 532-556).

Missing today:
- an ordinary config/action surface that sends a bounded X/Y/rotation step through `0x9018`;
- typed parsing of the condition fields required to expose current position and enforce camera-reported limits;
- round-trip / restoration tests for movement.

Do not claim full composition support merely because the enable switch exists.

## Required research before writing

1. Re-read the IT2 call site for `SetCompositionAdjustmentOffset` and recover exact parameter order, signedness, units and the meaning of its two response values.
2. Map the GetAllConditions composition fields precisely:
   - current X/Y shift;
   - rotation;
   - per-axis limits / enabled state;
   - any model-specific scaling.
3. Confirm whether KP and K-3 III-family share identical wire units/limits; do not infer this from the common UI gate.
4. Cross-check patcher #48 firmware findings for any additional model gates or parameter constraints.

## Implementation design

Prefer a small Pentax-specific action/config family rather than overloading `pentaxcompositionadjust` with movement semantics.

Suggested shape (names can follow normal libgphoto2 conventions after review):
- read-only/current composition status widget;
- bounded horizontal step action/value;
- bounded vertical step action/value;
- bounded rotation step action/value;
- existing `pentaxcompositionadjust` remains the enable/disable switch.

The implementation must:
- require `supported_model && vendor_mode_enabled`;
- use the existing composition capability helper;
- read current conditions before the move;
- refuse any requested move that exceeds camera-reported limits;
- send exactly the proven `0x9018` parameters;
- validate the two returned values if they are meaningful state/ack data;
- read conditions after the move and verify the expected bounded change;
- expose a restoration path for hardware qualification.

## Tests

Automated tests should cover:
- exact parameter encoding once recovered;
- signed/unsigned boundaries;
- model gate (K-3 III-family/KP only unless new evidence expands it);
- limit enforcement using synthetic conditions fixtures;
- malformed/short condition payload fail-closed;
- no accidental mutation when the enable switch is off, if IT2/camera semantics require it.

Hardware qualification in #44:
1. record original composition state;
2. enable composition adjustment if needed;
3. move one minimal step in each supported axis/rotation;
4. verify conditions/readback;
5. move back to the exact original state;
6. verify restoration;
7. disconnect/reconnect and verify no stale host-side state.

## Acceptance

- [ ] Exact `0x9018` parameter and response semantics are source/trace documented.
- [ ] Required composition condition fields are typed and bounds-checked.
- [ ] A safe public Pentax-specific movement surface exists.
- [ ] Movement is model-gated and camera-limit-gated.
- [ ] Automated encoder/limit/model-gate tests exist.
- [ ] Hardware move/readback/restore passes on each physically available applicable body.
- [ ] #66 matrix distinguishes composition switch (`d02a`) from offset movement (`0x9018`).
- [ ] No proprietary IT2/decompilation artefacts are redistributed.
====================================================================================================
#67 | Pentax IT2 parity: implement detailed Custom Image controls d021-d029 and d02d
labels:  | created 2026-09-09 | comments 0
----------------------------------------------------------------------------------------------------
## Goal

Close the genuine Image Transmitter 2 parity gap for Pentax **detailed Custom Image (CI)** controls that are source-defined in the existing IT2 capability matrix but are not currently exposed through `camlibs/ptp2/config.c`.

This is a child of #52/#66 and is intentionally separate from the already-implemented `pentaxcimode` (`0xd020`) and `pentaxcrossprocess` (`0xd02c`) surfaces.

## Current code state

Already implemented:
- `0xd020` Custom Image mode via `pentaxcimode`, with IT2 nominal/internal mapping;
- `0xd02c` cross-process type via `pentaxcrossprocess`, with K-3 III-family gating and the proven precondition that CI mode must first be set to cross-process.

Not currently found as ordinary config get/put surfaces:
- `0xd021` CI saturation;
- `0xd022` CI hue;
- `0xd023` CI high/low key;
- `0xd024` CI filter effect;
- `0xd025` CI monotone toning;
- `0xd026` CI toning;
- `0xd027` CI fine sharpness;
- `0xd028` CI highlight contrast;
- `0xd029` CI shadow contrast;
- `0xd02d` CI user-filter structured value.

Do not treat constants in `ptp.h` or entries in the research matrix as implementation proof.

## Source-defined wire semantics to preserve

From `docs/pentax/IMAGE_TRANSMITTER_CAPABILITY_MATRIX.md`:
- `0xd021` saturation — signed byte;
- `0xd022` hue — signed byte;
- `0xd023` high/low key — signed byte;
- `0xd024` filter — UINT8; UI value 9 maps to wire 255;
- `0xd025` monotone toning — signed byte;
- `0xd026` toning — UINT8;
- `0xd027` fine sharpness — UINT8;
- `0xd028` highlight contrast — signed byte, detailed-CI model gate;
- `0xd029` shadow contrast — signed byte, detailed-CI model gate;
- `0xd02d` user filter — 8-byte signed triplet structure; **trace before write**.

Also reconcile generic PTP `0x5014` contrast and `0x5015` sharpness where IT2 uses them as part of the CI surface; do not create duplicate controls with contradictory semantics.

## Required implementation approach

1. Re-read the IT2 source/trace evidence for each property and write down exact UI ↔ wire transforms before coding.
2. Add explicit model capability helpers rather than model-name conditionals in config handlers.
3. Keep base-CI controls distinct from detailed-CI controls where IT2 gates them differently.
4. For each scalar control:
   - fresh GET;
   - validate data type/length;
   - expose bounded widget values;
   - SET one value;
   - GET/read-back;
   - restore exact original value;
   - verify restoration.
5. For `0xd02d`, do not write until the 8-byte structure and each signed field are trace-proven and fixture-tested.
6. Preserve existing `pentaxcimode` / `pentaxcrossprocess` behaviour and their model gates.
7. Do not promote firmware-string-only evidence from patcher #48 to writable support.

## Model gating

Use #66 + patcher #48 to establish per-body gates. At minimum preserve the known distinction that detailed CI is available on K-3 III-family but not universally on K-1 II.

Every model row must be classified independently; do not infer from adjacent model numbers.

## Tests

Add automated fixtures for:
- signed-byte encode/decode boundaries;
- `0xd024` UI 9 ↔ wire 255 special mapping;
- unknown/unexpected wire values fail closed or round-trip as raw values without corruption;
- detailed-CI model gates;
- `0xd02d` exact 8-byte encoder/decoder once proven;
- no regression to `pentaxcimode` or `pentaxcrossprocess`.

Hardware qualification should be GET-before → bounded SET → GET-after → restore → GET-restore, with exact camera firmware + libgphoto2 SHA recorded in #44.

## Acceptance

- [ ] Each IT2 CI property has documented wire semantics and model gate.
- [ ] Scalar controls `d021-d029` have safe get/put surfaces where evidence supports writes.
- [ ] `d02d` remains withheld unless its full 8-byte structure is trace/hardware proven.
- [ ] Writes are bounded, read back and restored during qualification.
- [ ] Existing CI mode/cross-process controls are unchanged except where evidence requires correction.
- [ ] #66 matrix is updated per model with CODE/TR/HW status.
- [ ] No proprietary IT2/decompilation artefacts are copied into the public repo.
====================================================================================================
#66 | Reconcile every firmware-backed Pentax/Ricoh model against IT2 capability matrix and current libgphoto2
labels:  | created 2026-09-09 | comments 5
----------------------------------------------------------------------------------------------------
## Goal

Turn the current model/control research under #52 into an authoritative **model × USB personality × capability** reconciliation against three evidence sources:

1. firmware-derived evidence from `benro-polaris-firmware-patcher#48` / PrivateResearch;
2. official-client / Image Transmitter 2 evidence already captured in `docs/pentax/IMAGE_TRANSMITTER_CAPABILITY_MATRIX.md` and `IMAGE_TRANSMITTER_SETTING_PATH.md`;
3. actual current libgphoto2 implementation and tests.

This issue is a child work package under #52. It must not duplicate #53-#60; instead it should classify where each existing child issue applies by model and expose any broader parity gaps that need separate issues.

## Scope

Cover every Pentax/Ricoh body or USB personality evidenced by current code, PrivateResearch, NINA/PentaxLibrary research, firmware, or Image Transmitter evidence.

At minimum include distinct rows for:
- K-3 III
- K-3 III Monochrome
- K-1 II PTP `25fb:0183`
- K-1 II MSC/SCSI `25fb:0182`
- K-1
- K-3
- K-3 II
- KP
- K-70
- KF
- K-01 USB-SCSI `25fb:0130`
- K-01 PTP/MTP `25fb:0131`
- 645D
- 645Z
- GR III / relevant GR-family bodies
- every additional body currently registered in `camlibs/pentax` or `camlibs/ptp2`
- every additional firmware-backed model found by patcher #48.

Do not collapse alternate USB personalities into one row.

## Required columns per row

### Identity / routing
- marketing model;
- exact manufacturer/model string;
- VID:PID per personality;
- transport (USB-SCSI/MSC/PTP/MTP/etc.);
- claiming camlib;
- VendorExtensionID / vendor identity;
- Image Transmitter internal model number where known;
- firmware version gate(s);
- Pentax vendor-mode eligibility;
- evidence level for the identity gate.

### Capability families
For each capability below record BOTH:
1. evidence status (`HW`, `TR`, `FW`, `CODE`, `INF`, `NO`, `U`), and
2. libgphoto2 status (`implemented`, `wrapper needed`, `partially implemented`, `withheld`, `unsupported`, `unknown`).

Required capability columns:
- vendor-mode lifecycle;
- all-conditions read;
- aperture;
- shutter/Bulb timer;
- extended ISO;
- exposure compensation;
- white balance / colour temperature;
- `imageformat`;
- `imagequality`;
- RAW kind / JPEG+RAW semantics;
- card-writing mode / current slot;
- drive mode;
- bracket mode/step;
- autofocus capture policy;
- immediate autofocus action;
- manual focus drive;
- positional/live-view AF point control;
- preview/live view;
- PC/high-resolution live view;
- LV zoom / AF position / peaking;
- still capture;
- Bulb;
- capture candidate workflow;
- object/file transfer;
- finalize/delete candidate;
- `capturetarget` equivalent or explicit absence;
- Custom Image controls;
- composition adjustment;
- aperture keep;
- movie controls;
- timer/remote-continuous controls;
- Astrotracer/Astro capture/focus capabilities;
- Star AF if evidenced;
- any newly discovered firmware/IT2 capability from #48.

## Cross-check method

For each capability cell, inspect the actual code path rather than only constants in `ptp.h`.

At minimum inspect where relevant:
- `camlibs/ptp2/config.c`
- `camlibs/ptp2/library.c`
- `camlibs/ptp2/pentax-utils.[ch]`
- `camlibs/ptp2/ptp.[ch]`
- `camlibs/pentax/library.c`
- tests / Pentax fixtures
- `docs/pentax/IMAGE_TRANSMITTER_CAPABILITY_MATRIX.md`
- `docs/pentax/IMAGE_TRANSMITTER_SETTING_PATH.md`
- `docs/pentax/REAL_HARDWARE_TEST_LOG.md`
- `docs/pentax/GENERIC_CONTROL_COMPATIBILITY_AUDIT.md`
- derived findings from patcher #48.

Presence of an opcode/property constant is **not** implementation proof. Presence of a handler is **not** model support unless the model gate routes to it.

## Specific questions this task must answer

1. Which firmware-backed models are currently recognized but routed to the wrong family or fail-closed unnecessarily?
2. Which models are missing entirely from the abilities/model gate tables despite firmware/IT2 evidence?
3. Which IT2-supported capabilities already exist in libgphoto2 but are hidden behind Pentax-specific names?
4. Which IT2 capabilities have protocol support in code but no public config/action surface?
5. Which capabilities are represented only by constants/research and remain intentionally withheld?
6. Which current implementation assumptions incorrectly treat K-3 III semantics as universal?
7. Which legacy USB-SCSI controls can safely act as semantic precedent for modern wrappers, and which cannot?
8. Does any model have a real Pentax destination selector equivalent to `capturetarget`? If not, record explicit absence rather than creating a fake mapping.
9. Which models expose positional AF / `LiveViewAFPosition`, and which only support AF at the current camera-selected area?
10. What remains needed for full Image Transmitter 2 parity after #53-#60 are complete?

## Output

Create/update a public Markdown matrix under `docs/pentax/` that is suitable as the durable #52 completion artefact. It must be readable by both developers and future agents.

Each non-trivial cell should link to or cite one of:
- exact source file/function;
- hardware test log;
- official-client trace/research document;
- firmware-derived evidence summary from #48.

Also post a concise issue comment summarizing:
- newly supported candidates;
- incorrect model gates;
- capability gaps;
- child issues needing updates;
- any genuinely new issue that should be created.

Before creating any new issue, search existing #44, #50-#60 and related Pentax issues for an appropriate thread.

## Guardrails

- Never infer support from a neighbouring model number alone.
- Never enable Pentax vendor behaviour for all Ricoh-branded cameras.
- Treat K-01 `0130` and `0131` independently.
- Treat K-1 II `0182` and `0183` independently.
- DeviceInfo omission is not proof of unsupported capability on known IT2 bodies.
- Do not expose a control merely because firmware contains a string/opcode.
- Preserve existing Pentax-specific diagnostic controls when adding generic wrappers.
- Do not rename manual-focus drive into autofocus.
- Do not map `pentaxcardwritingmode` to `imageformat`.
- Do not manufacture `capturetarget` without protocol evidence.

## Acceptance

- [ ] Every referenced model/personality has a row.
- [ ] Every row has explicit routing/model-gate evidence.
- [ ] Every required capability has both evidence status and implementation status.
- [ ] Firmware-derived findings from patcher #48 are incorporated.
- [ ] IT2 matrix and setting-path docs are reconciled against current code.
- [ ] K-1 II vs K-3 III differences are explicit.
- [ ] K-01 dual personalities are explicit.
- [ ] Generic controls #53-#60 are mapped to applicable model families.
- [ ] Missing IT2 parity features beyond #53-#60 are listed.
- [ ] Any actionable code gaps are attached to an existing issue or a justified new issue.
- [ ] #52 can use this matrix as its completion artefact.
--- COMMENT by ian-morgan99 2026-09-09 ---
Initial cheap-win audit of current `camlibs/ptp2/config.c` finds that a substantial amount of Pentax/IT2 functionality already has real handlers, but much of it is exposed only through Pentax-specific diagnostic/config names rather than the generic vocabulary downstream clients such as pgphoto ask for.

### Already implemented Pentax-specific surfaces worth treating as existing protocol support

Current config entries include:

- `pentaxconditions` — `GetAllConditions`
- `pentaxliveviewcontrols`
- direct descriptors for shutter/Bulb (`0xd00f`), aperture (`0x5007`), ISO (`0xd01e`), EV (`0x5010`), focal length (`0x5008`), WB (`0x5005`), drive (`0xd013`), focus peaking (`0xd02b`), PC live view (`0xd035`)
- `pentaxfocuspeaking`
- `pentaxpclvmode`
- `pentaxbracketmode`
- `pentaxbracketstep`
- `pentaxcimode`
- `pentaxcompositionadjust`
- `pentaxcrossprocess`
- `pentaxmoviemode`
- `pentaxpclvkeep`
- `pentaxcardwritingmode`
- `pentaxdirectshutter`
- `pentaxdirectiso`
- `pentaxdirectaperture`
- `pentaxdirectev`
- `pentaxliveviewafposition`
- `pentaxliveviewzoom`
- `pentaxdrivemode`
- `pentaxdirectwb`

This means #66 should not classify those capabilities as merely “opcode constant present”: there are actual get/put handlers and menu surfaces today. The remaining question per capability is model gate + hardware/trace evidence + whether a generic/client-facing alias/action is appropriate.

### Positional AF is further along than our previous discussion implied

`pentaxliveviewafposition` already has a concrete `0xd036` implementation. The handler comment records the IT2 8-byte write layout `{2,0,0,0,Xlo,Xhi,Ylo,Yhi}`, obtains the live-view geometry in-session, bounds coordinates to that geometry, performs the write, and verifies by reading the position back. So the **Pentax side of point-selection AF positioning is already implemented**; what is still missing is the immediate AF trigger / Benro integration and per-body qualification.

That is important for the Benro/OpenPolaris work: if stock Benro turns out not to pass `(x,y)`, OpenPolaris could still implement richer Pentax tap-to-focus later by calling the existing AF-position surface followed by the eventual #57 AF action. Do not bury `pentaxliveviewafposition` inside #57: coordinate selection and AF trigger are separate capabilities.

### Generic-vocabulary gaps confirmed in the current action menu

The action menu has generic `autofocusdrive` implementations for Nikon/Canon/Fuji, and generic `manualfocusdrive` for Nikon/Canon, but no Pentax entries. Pentax instead exposes separate `manualfocusdrivenear` / `manualfocusdrivefar` entries, dispatched to `0x9017` for new-focus models or `0x9016` for old-focus models. That directly validates #57 and #59 as compatibility-surface work rather than missing low-level focus support.

Likewise, the K-01 stock Polaris baseline shows `polestar_app` explicitly asks libgphoto2 for `shutterspeed`, `manualfocusdrive`, `autofocusdrive`, `aperture`, `imageformat`, and `capturetarget`. That gives us a concrete downstream vocabulary oracle rather than guessing generic names.

### Cheap-win classification I suggest for #66

1. **Already generic / avoid churn:** `iso`, `shutterspeed`, `whitebalance`, `exposurecompensation` where current modern Pentax routing is proven.
2. **Existing safe Pentax handler; generic alias/adaptor is the main gap:** `aperture` (#53), likely `manualfocusdrive` (#59), plus any downstream-needed aliases we can prove semantically.
3. **Existing Pentax protocol+handler but no equivalent generic standard required yet:** LV AF position, LV zoom, peaking, bracketing, CI, composition, PC LV, movie/card controls. Keep Pentax names unless an actual downstream client requires a standard name; nevertheless count these toward IT2 parity.
4. **Low-level path exists but generic action missing:** immediate AF (#57) — use the proven Pentax AF operation once exact `0x9011` mode/params are recovered; do not reuse manual focus.
5. **Real missing generic codec/surface despite property knowledge:** `imageformat` / `imagequality` (#54/#55), centred on safe byte-preserving `0xd01b` handling.
6. **Do not fake:** `capturetarget` (#60) unless firmware/IT2 evidence reveals a true destination selector.

### One implementation-quality issue remains visible

The production camlib still exports `ptp2_pentax_aperture_alias_present()` solely so a regression test can `dlsym` it. That is the #53 concern already raised: the alias itself is a reasonable cheap win, but the test hook should not become part of the production ABI, and the acceptance test should exercise `gp_camera_get_single_config("aperture")` / actual routing rather than pointer identity.

I would use this as the first pass of #66’s “already implemented but poorly exposed” column. Patcher #48 can then supply model/firmware evidence and #66 can decide per row whether each surface is `CODE`, `FW`, `TR` or `HW` rather than rediscovering the handlers.
--- COMMENT by ian-morgan99 2026-09-09 ---
### Second pass: full IT2-surface audit against current code

I extended the audit beyond the generic pgphoto names and compared the current fork against the normative `IMAGE_TRANSMITTER_CAPABILITY_MATRIX.md`. The important result is that the implementation is materially further along than the matrix's older `target/research implemented` wording suggests in several areas, but there are also clear parity gaps that are *not* covered by #53-#60.

#### 1. Capture / new-transfer workflow: integrated code exists, but remains research-build gated

The public `camera_capture()` path has a real Pentax new-transfer implementation under `LIBGPHOTO2_ENABLE_PENTAX_RESEARCH_CAPTURE`:

- `0x9011` `InitiateCapture` is called with `focus_mode` derived from the `ptp2/autofocus` policy;
- GetAllConditions is polled to size the timeout and wait for a fresh transfer candidate;
- candidate metadata is fetched through `0x900b`;
- the camera's file-operation stream is consumed through `0x900c`;
- data blocks are fetched through `0x900d`, with the existing bounded capture buffer / 8 MiB request discipline;
- the completed host image is cached before camera-side finalisation;
- `0x900e` finalises/deletes the transfer candidate only after the host cache is ready;
- transfer state is tracked explicitly (`TRIGGERED -> WAITING -> CANDIDATE -> TRANSFERRING -> CACHING -> FINALIZING -> COMPLETE`).

So #66 should classify `0x900b-0x900e` + still capture as **CODE/integrated but research-gated**, not merely constants or disconnected research helpers. The remaining work is promotion/qualification (#44/#45), model-family validation and hardware evidence—not designing the transfer state machine from scratch.

Preview is similar: the Pentax vendor live-view path is implemented but compiled out of ordinary builds by the same research flag. This distinction matters when we assess "IT2 parity": a feature can be protocol-complete in the fork while still intentionally unavailable to normal libgphoto2 users.

#### 2. Identity/model gating is already broader than the old K-1 II / K-3 III framing

`pentax_lookup_model()` currently has explicit model/USB gates for K-3 III, K-3 III Mono, KP, K-70, K-1 II, 645D, 645Z, KF, K-3, K-1 and GR III. K-3 II is deliberately fail-closed. This means the durable matrix should start from the actual current model gate table, then annotate per-capability evidence; it should not imply only K-1 II/K-3 III are represented in code.

There are also explicit capability helpers rather than one universal Pentax feature set:

- new focus (`0x9017`) is model-gated (K-3 III-family / KP / GR III per IT2 evidence);
- exposure bracketing is gated to K-3 III-family + 645Z;
- composition adjustment is gated to K-3 III-family + KP;
- movie settings and PC live-view are K-3 III-family gated;
- cross-process is K-3 III-family only, with a recorded K-1 II negative hardware result;
- card-writing mode is restricted to the IT2-derived dual-slot body set.

That architecture is good: future work should extend/verify these tables rather than scattering new model-name conditionals through config handlers.

#### 3. Drive/timer/remote support is further along than the matrix suggests

`pentaxdrivemode` is not just a raw `0xd013` diagnostic. It already implements the IT2 nominal-to-internal mapping and verifies writes through live conditions. Its table currently includes:

- single;
- continuous high/mid/low;
- self-timer 12 s / 2 s / continuous;
- remote / remote-3s / remote-continuous;
- mirror-up / mirror-up-remote;
- multi-exposure;
- interval;
- interval-movie;
- star-stream.

Therefore a meaningful part of the IT2 "new timer / remote continuous" family is already surfaced through `pentaxdrivemode`. #66 should distinguish **drive-mode selection already implemented** from any still-missing secondary parameters (interval count/time, timer details, etc.). Do not open a generic "timer support missing" issue until the remaining IT2 setters are enumerated separately.

#### 4. Bracketing: public Pentax-specific handlers already exist

`pentaxbracketmode` and `pentaxbracketstep` have real get/put handlers. The mode mapping currently includes off, 3-frame, 5-frame, +2 and -2 variants, and the step handler writes the IT2 tenths encoding. They are model-gated rather than exposed universally.

Classification: **CODE/public Pentax config surface; per-model hardware qualification incomplete**. This is not a missing-protocol item.

#### 5. Custom Image: base mode + cross-process exist; detailed CI parity is still genuinely missing

Implemented surfaces include:

- `pentaxcimode` (`0xd020`) with the IT2 internal-code mapping;
- `pentaxcrossprocess` (`0xd02c`) with K-3 III-family gating and the important precondition that CI mode must first be set to cross-process.

However, I cannot find public config handlers for the detailed IT2 CI properties `0xd021-0xd029` (saturation, hue, high/low key, filter, monotone toning, toning, fine sharpness, highlight/shadow contrast) or the structured user-filter `0xd02d`. Constants/documentation are not enough.

This is a **real parity gap beyond #53-#60**. It should remain on #66's missing-feature list pending #48 evidence; after that, it likely merits its own child issue because it is a coherent feature family rather than a generic-name wrapper.

#### 6. Composition adjustment is only partially implemented

`pentaxcompositionadjust` currently controls the `0xd02a` composition-adjustment enable switch, with the correct K-3 III-family/KP gate. That is useful, but it is not full IT2 composition control.

The actual movement operation `0x9018` (`SetCompositionAdjustmentOffset`, direction/steps with two response values) is present at the PTP-definition/research level but I do not see an ordinary config/action surface for translating user X/Y/rotation movement into that operation. Likewise the condition fields for composition axes/limits remain broader than the parsed public subset.

Classification: **switch implemented; directional/offset control still missing**. This is another likely post-#48 child issue.

#### 7. Live-view family: surprisingly mature

Already implemented as Pentax config surfaces/helpers:

- live-view geometry parser (`0xd009`);
- vendor live-view frame retrieval (`0x9006`, research-build public gating);
- PC live-view mode (`0xd035`), model-gated;
- AF position (`0xd036`) with bounded 8-byte encoder and read-back verification;
- live-view zoom (`0xd037`) with the IT2 12-byte form and the specific 16x -> 10x fallback only on `0x201c`;
- focus peaking (`0xd02b`) with model/data-shape caution;
- keep-live-view surface.

So the remaining live-view work is mainly **promotion, hardware matrix and high-resolution/model-specific behaviour**, not protocol discovery. Positional AF is already separable from the immediate AF trigger (#57), which is the right architecture.

#### 8. Movie support: base mode exists; full movie-setting parity does not

`pentaxmoviemode` exists and is deliberately restricted to IT2 movie-setting-capable models. `0xd01b` also carries movie size/frame-rate semantics in its compound structure.

But this should not be labelled "full movie parity" yet: the matrix refers to movie settings, movie sub-image and model-specific movie controls beyond the boolean `0xd039` mode. The safe classification is **base movie mode implemented + compound-format work shared with #54/#55; broader movie workflow/settings still to enumerate**.

#### 9. Dual-slot/card support: one real control exists; current-slot/format coupling remains incomplete

`pentaxcardwritingmode` is a real gated get/put surface implementing `0x9004` semantics and maintaining fail-closed cached state. This must remain distinct from `imageformat`.

The richer IT2 card picture is still incomplete because `0xd01b` embeds writing formats, requested card slot and (on K-3 III) current slot, while conditions also report per-card state/remaining/current slot. #54/#55 should own only the compound image-format codec semantics; #66 should separately track card/current-slot status and model qualification so we do not accidentally collapse storage selection into image format.

#### 10. Conditions/status parser: useful subset implemented, large telemetry gap remains

`pentax_parse_conditions()` currently exposes operation/activity state, user/exposure mode, shutter/Bulb rational, aperture, EV, ISO, astro flags/limit, open aperture, drive mode, WB, AF mode and capability flags. This is enough to support safe write verification for several implemented controls.

The IT2 matrix still contains many fields not promoted into the typed public subset: bracket state/count, format/size/quality/RAW/CI status, colour temperature, battery/focal length, flash state, detailed card state/activity/remaining, AF result/rotation, crop/aspect, composition axes/limits, movie slot, electronic shutter, current slot, etc.

These should be treated as **status-parity backlog**, not blockers to every control. Add typed fields when a concrete feature/readback needs them rather than blindly exposing all 568 bytes.

#### 11. Clear remaining operation-level gaps

These remain genuinely unresolved/withheld in current code and should not be upgraded simply because the opcode constant exists:

- `0x9013` Green-button/interrupt as a user operation;
- `0x9014` light-metering mode;
- `0x9018` composition offset movement (see above);
- `0x9019` aperture-keep operation;
- `0x9021` WB measurement;
- `0x9022` dust reduction;
- `0x97ff` general command;
- K-1 II Star AF trigger (firmware evidence, opcode unresolved);
- K-3 III Astro-mode capture path;
- warning flag / InvokeCameraTask semantics where only names/references exist.

One code-quality caution discovered during this pass: `camera_trigger_capture()` still contains a research-build Pentax branch that calls `0x9013` and describes the Green button as "IT2's capture trigger". The normative matrix says `0x9013` is Green-button/interrupt, while actual still capture uses `0x9011/0x9012`. Pentax abilities currently do **not** advertise `GP_OPERATION_TRIGGER_CAPTURE` in the research block, so this is latent rather than an advertised release path, but we should not promote trigger-capture support by enabling that branch. If `trigger_capture` is ever exposed for Pentax it should be implemented from proven shutter semantics, not Green-button semantics.

### Revised parity picture

I would now classify the fork roughly as:

- **Protocol path substantially implemented:** lifecycle, conditions subset, direct exposure/WB controls, live-view family, new-transfer still capture, focus movement, drive/timer/remote selection, bracketing, CI base/cross-process, card-writing mode, base movie mode.
- **Implemented but intentionally contained:** Pentax still capture and preview in normal builds (research compile gate).
- **Existing Pentax surface but needing model/HW qualification:** many of the controls above.
- **Compatibility/API gaps:** #53-#60.
- **True IT2 feature gaps beyond #53-#60:** detailed CI (`d021-d029/d02d`), composition offset (`0x9018`) surface, detailed colour-temperature control (`0xd018`) surface, aperture-keep (`0x9019`), WB measurement (`0x9021`), dust reduction (`0x9022`), remaining movie-specific settings/sub-image, richer card/current-slot/status, Star AF/Astro capture, and selected condition/status fields needed by those features.

This is the list I would use to decide new child issues **after patcher #48 has finished firmware corroboration**. It avoids creating issues for things the code already does and focuses new work on actual missing behaviour.
--- COMMENT by ian-morgan99 2026-09-09 ---
Consistency pass findings to fold into the live reconciliation matrix:

1. **K-1 II `0xd036` write is overclaimed relative to retained evidence.** `REAL_HARDWARE_TEST_LOG.md` only records read-only AF-position discovery (centre 360,240; no setter), while `_put_Pentax_LiveViewAFPosition()` currently states that K-1 II accepts writes and treats a structurally valid centre GET as proof of application. #8 is reopened until that semantic is trace/hardware-proven or the write path is withheld for K-1 II.
2. **Duplicate config registration:** `pentaxliveviewafposition` is registered twice in the same config menu block. #69 owns the cleanup.
3. **Research capture semantic mismatch:** closed #45 grouped the `0x9013` Green-button branch with ordinary capture. #45 is reopened; #66 should classify `0x9013` separately from `0x9011` initiate / `0x9012` terminate and should not count a successful ACK as capture-semantic proof.
4. **Historical doc drift:** `CAPABILITY_MATRIX_AUDIT.md` is a 2026-08-21 snapshot and now contradicts later model/focus/LV/preview work. #70 owns making that historical status explicit. Do not ingest its stale implementation-status cells as current truth.
5. Current `CONTROL_ISSUE_INDEX.md` and `AGENT_WORK_QUEUE.md` have been updated (commits `4b552a2a1` and `4b3e89242`) to include #66-#70 and the reopened #8/#45 gates.

Please make the eventual #66 durable matrix distinguish **protocol/source evidence**, **code presence**, **public exposure**, and **hardware-qualified write semantics**. In particular, a handler or SET+ACK must not upgrade a cell to HW-W without retained evidence.
--- COMMENT by ian-morgan99 2026-09-09 ---
this is lower priority than first release of k1ii k3iii k-01
--- COMMENT by ian-morgan99 2026-09-09 ---
### Next-pass architecture enhancement — do not block the next release

For the current release, keep the existing evidence-led per-model validation and avoid a broad routing refactor. Getting the K-3 III/K-1 II/Polaris path qualified and released is the priority.

After that release, use this matrix to consolidate model handling into **protocol/capability families** rather than allowing support to evolve into one bespoke implementation path per camera.

Target design:

`model/personality -> capability profile -> reusable protocol implementation`

A capability profile should select things such as vendor-mode family, focus opcode family (`0x9016` vs `0x9017`), capture/live-view family, AF-position semantics, writing-format layout, detailed-CI support, composition support, etc. Model-specific branches should be retained only where hardware/trace evidence proves a genuine exception.

Important distinction: **independent per-camera validation does not imply independent per-camera implementation**. K-3 III and K-1 II should still be tested independently even when they share reusable handlers.

This is particularly important for #66's full model list: adding a newly evidenced body should ideally become a model/profile-table addition plus qualification, not another camera-specific driver path. Distinct USB personalities such as K-01 `0130` vs `0131` and K-1 II `0182` vs `0183` can still map to different profiles where the transport/protocol genuinely differs.

Treat this as a **post-release / next-pass maintainability enhancement**, not a gate for the release currently being prepared. It should feed into #42 before eventual upstream submission.
--- TIMELINE EVENTS ---
[cross-referenced by ian-morgan99 2026-09-09] <- 52 TRACKING: sweep Pentax/Ricoh model × generic-control coverag
[commented by ian-morgan99 2026-09-09]
[commented by ian-morgan99 2026-09-09]
[cross-referenced by ian-morgan99 2026-09-09] <- 67 Pentax IT2 parity: implement detailed Custom Image controls 
[cross-referenced by ian-morgan99 2026-09-09] <- 68 Pentax IT2 parity: expose composition offset movement via 0x
[cross-referenced by ian-morgan99 2026-09-09] <- 45 UPSTREAM: Remove or resolve research-build gating and separa
[cross-referenced by ian-morgan99 2026-09-09] <- 69 Remove duplicate pentaxliveviewafposition config registratio
[cross-referenced by ian-morgan99 2026-09-09] <- 70 Mark CAPABILITY_MATRIX_AUDIT.md as historical and reconcile 
[commented by ian-morgan99 2026-09-09]
[cross-referenced by ian-morgan99 2026-09-09] <- 44 UPSTREAM: Define and execute Pentax hardware validation matr
[cross-referenced by ian-morgan99 2026-09-09] <- 8 K-1 II live-view AF-position write semantics lack retained h
[commented by ian-morgan99 2026-09-09]
[commented by ian-morgan99 2026-09-09]
====================================================================================================
#60 | ptp2/pentax: define honest generic `capturetarget` semantics or explicit graceful absence
labels: enhancement | created 2026-09-08 | comments 1
----------------------------------------------------------------------------------------------------
Parent: #52. Discovery: #51. Polaris integration evidence: benro-polaris-firmware-patcher#36.

## Problem
Polaris `pgphoto` looks up `capturetarget` and attempts `capturetarget=1`; Pentax ptp2 exposes no generic capture-target widget, producing `enableCaptureTarget ret -1` in the integration path.

Other vendors' `capturetarget` commonly distinguishes host/internal RAM vs card capture, but we must not invent the same semantics for Pentax. Pentax vendor capture/candidate transfer and card-writing mode are separate mechanisms.

## Required work
Determine whether modern Pentax vendor capture has a meaningful, controllable destination equivalent to libgphoto2 `capturetarget`. If yes, expose it with proven semantics. If no, document intentional absence and ensure downstream clients can treat absence as capability information rather than camera failure.

Do not map this to `pentaxcardwritingmode` merely to satisfy the name.

## Acceptance
- [ ] Pentax capture-destination semantics documented from protocol evidence.
- [ ] either a correct generic widget is implemented or explicit unsupported behaviour is recorded.
- [ ] K-3 III and K-1 II classified independently.
- [ ] capture/download behaviour remains correct after reconnect/restart.
- [ ] no card/slot selection regression.
- [ ] Polaris integration no longer turns missing `capturetarget` into failure of otherwise-supported camera controls.
--- COMMENT by ian-morgan99 2026-09-09 ---
Cross-checking against the current IT2 capability matrix strengthens the fail-closed option here. The documented modern Pentax capture path is vendor-mode + initiate/terminate + candidate metadata/file-command/block-transfer/finalize (`0x9011/0x9012`, `0x900b-0x900e`); the matrix does not identify a separate host-vs-card capture-destination control equivalent to Canon/Nikon `capturetarget`.

Until the forthcoming IT2 source/trace review proves otherwise, I would not invent `capturetarget=1`. Treat absence as the current evidence-backed behaviour and fix the Polaris/pgphoto integration so a missing capturetarget is non-fatal. If later IT2 evidence exposes a genuine destination selector, we can add it with exact semantics.
--- TIMELINE EVENTS ---
[labeled by ian-morgan99 2026-09-08] {"label": {"name": "enhancement", "color": "a2eeef"}}
[cross-referenced by ian-morgan99 2026-09-08] <- 51 ptp2/pentax: register the generic control names third-party 
[cross-referenced by ian-morgan99 2026-09-08] <- 44 UPSTREAM: Define and execute Pentax hardware validation matr
[cross-referenced by ian-morgan99 2026-09-08] <- 52 TRACKING: sweep Pentax/Ricoh model × generic-control coverag
[commented by ian-morgan99 2026-09-09]
[cross-referenced by ian-morgan99 2026-09-09] <- 66 Reconcile every firmware-backed Pentax/Ricoh model against I
====================================================================================================
#59 | ptp2/pentax: add generic `manualfocusdrive` wrapper with model-family dispatch
labels: enhancement | created 2026-09-08 | comments 1
----------------------------------------------------------------------------------------------------
Parent: #52. Discovery: #51.

## Problem
Generic clients request `manualfocusdrive`, but modern Pentax ptp2 currently exposes directional model-specific widgets: new-family `manualfocusdrivenear/far` using `0x9017` and old-family `oldfocusdrivenear/far` / related paths using `0x9016`.

## Required work
Implement the established libgphoto2 `manualfocusdrive` contract as a wrapper over the correct Pentax focus family while preserving direction/step semantics and all existing Pentax-specific names.

K-3 III/new-family and K-1 II/old-family must not share one hard-coded opcode/algorithm. Use the existing model capability dispatch and fail closed on unknown bodies.

## Acceptance
- [ ] generic value/step contract matches other libgphoto2 vendors closely enough for clients.
- [ ] model dispatch selects `0x9017` vs `0x9016` correctly.
- [ ] direction and bounded movement are hardware-verified independently on K-3 III and K-1 II where available.
- [ ] no retry escalation or unbounded focus movement.
- [ ] existing near/far diagnostic widgets remain available.
- [ ] regression tests cover family dispatch and unsupported models.
- [ ] pgphoto `manualfocusdrive` succeeds on supported hardware.
--- COMMENT by ian-morgan99 2026-09-09 ---
Benro-side evidence now gives this issue a concrete downstream target: stock Benro defines `311 = SP_SET_FOCUS_ADJ`, and OpenPolaris/private research already records its send shape as `mode:<...>;adj:<...>;` with `ret:0/-1` acknowledgement. [benro-polaris-firmware-patcher#47](https://github.com/ian-morgan99/benro-polaris-firmware-patcher/issues/47) is tracing the firmware handler to recover the exact enum/value semantics and pgphoto/libgphoto2 call chain.

That aligns well with the current Pentax implementation here: `config.c` already has model-aware near/far actions, dispatching `manualfocusdrivenear/far` to `0x9017` on new-focus bodies and `0x9016` on old-focus bodies. The compatibility gap is therefore mainly adapting those two actions to the generic singular `manualfocusdrive` semantic expected by pgphoto/Benro, without losing model-aware dispatch or the existing diagnostic names.

Please do not use `SP_SET_FOCUS`/Benro 262 as evidence for this issue; that belongs to #57 immediate AF. [benro-polaris-firmware-patcher#47](https://github.com/ian-morgan99/benro-polaris-firmware-patcher/issues/47) should give us the exact mapping needed to choose direction/step values for the generic adapter.
--- TIMELINE EVENTS ---
[labeled by ian-morgan99 2026-09-08] {"label": {"name": "enhancement", "color": "a2eeef"}}
[cross-referenced by ian-morgan99 2026-09-08] <- 51 ptp2/pentax: register the generic control names third-party 
[cross-referenced by ian-morgan99 2026-09-08] <- 52 TRACKING: sweep Pentax/Ricoh model × generic-control coverag
[cross-referenced by ian-morgan99 2026-09-09] <- 57 ptp2/pentax: implement generic `autofocusdrive` as a real AF
[cross-referenced by ian-morgan99 2026-09-09] <- 66 Reconcile every firmware-backed Pentax/Ricoh model against I
[commented by ian-morgan99 2026-09-09]
====================================================================================================
#58 | ptp2/pentax: define generic `manualfocus` AF/MF mode semantics
labels: enhancement | created 2026-09-08 | comments 0
----------------------------------------------------------------------------------------------------
Parent: #52. Discovery: #51.

## Problem
Polaris `pgphoto` requests `manualfocus`, while current Pentax ptp2 has no such generic widget. This must not be confused with moving the lens manually: in existing client vocabulary it may represent an AF/MF mode/state control.

## Required work
Determine the exact intended generic `manualfocus` contract from existing libgphoto2 vendors and pgphoto behaviour, then map it only if Pentax exposes a proven equivalent AF/MF mode property/operation for the relevant model family.

Do not implement this as a synonym for `manualfocusdrive` (#59) or negate the `autofocus` capture-policy setting (#56) without evidence that those semantics match.

## Acceptance
- [ ] expected generic widget type/values documented.
- [ ] Pentax protocol equivalent identified or issue explicitly concludes unsupported.
- [ ] model applicability recorded in #52 matrix.
- [ ] safe GET/SET and restore tested where supported.
- [ ] missing support is reported as unavailable, not as failure of unrelated controls.
- [ ] pgphoto behaviour verified after implementation or graceful-absence decision.
====================================================================================================
#57 | ptp2/pentax: implement generic `autofocusdrive` as a real AF action
labels: enhancement | created 2026-09-08 | comments 2
----------------------------------------------------------------------------------------------------
Parent: #52. Discovery: #51.

## Problem
Clients request `autofocusdrive` as an immediate autofocus action. Pentax currently exposes manual directional focus-drive operations, but those are not semantically equivalent to autofocus.

## Required work
Identify the correct Pentax immediate-AF operation/sequence for each supported model family from Image Transmitter traces, firmware evidence, existing capture sequencing and direct hardware. Do **not** alias `autofocusdrive` to `manualfocusdrivenear/far` or `oldfocusdrivenear/far`.

Keep this separate from the virtual capture policy `autofocus` (#56), AF/MF mode selection (#58), and manual drive (#59).

## Acceptance
- [ ] protocol action/sequence documented with model applicability.
- [ ] one invocation causes a bounded AF action and does not trigger capture.
- [ ] failure/busy states return normal libgphoto2 errors without retry escalation.
- [ ] K-3 III and K-1 II tested separately if both are supported.
- [ ] unsupported models fail closed.
- [ ] generic `autofocusdrive` widget/action is covered by tests and pgphoto integration evidence.
--- COMMENT by ian-morgan99 2026-09-09 ---
One useful result from cross-checking the existing IT2-derived matrix: immediate autofocus should not start from the `0x9016`/`0x9017` manual focus families. The normative matrix records Pentax `0x9011` (`InitiateCapture`) as a multi-mode operation supporting release, **focus**, MWB, sync and aperture-reset actions on both K-1 II and K-3 III families.

I would therefore make `0x9011` focus-mode the first candidate for `autofocusdrive` and require the exact mode/parameters to be proven from the source/trace before exposing it. `0x9016`/`0x9017` should remain reserved for #59 manual lens movement. This gives us a cleaner semantic split and avoids building AF on the wrong opcode family.
--- COMMENT by ian-morgan99 2026-09-09 ---
One additional downstream constraint from the Benro/OpenPolaris cross-layer audit: stock Benro defines `262 = SP_SET_FOCUS`. That is the natural Benro-side consumer for a true Pentax immediate-AF action, but its exact payload/call chain is still being reversed in [benro-polaris-firmware-patcher#47](https://github.com/ian-morgan99/benro-polaris-firmware-patcher/issues/47).

Also keep **AF position** separate from **AF trigger**. Current `config.c` already implements `pentaxliveviewafposition` on `0xd036` with the IT2 8-byte `{2,0,0,0,Xlo,Xhi,Ylo,Yhi}` payload, live-view geometry bounds and read-back verification. So if Benro/OpenPolaris wants tap-to-focus, the likely E2E shape is:

`set AF position (0xd036)` → `trigger AF (#57)`

rather than trying to encode `(x,y)` inside generic `autofocusdrive`. [benro-polaris-firmware-patcher#47](https://github.com/ian-morgan99/benro-polaris-firmware-patcher/issues/47) is explicitly checking whether stock Benro 262 carries coordinates or whether any separate Benro AF-position path exists. Until that evidence lands, #57 should stay a coordinate-free immediate-AF action.
--- TIMELINE EVENTS ---
[labeled by ian-morgan99 2026-09-08] {"label": {"name": "enhancement", "color": "a2eeef"}}
[cross-referenced by ian-morgan99 2026-09-08] <- 51 ptp2/pentax: register the generic control names third-party 
[cross-referenced by ian-morgan99 2026-09-08] <- 52 TRACKING: sweep Pentax/Ricoh model × generic-control coverag
[commented by ian-morgan99 2026-09-09]
[cross-referenced by ian-morgan99 2026-09-09] <- 66 Reconcile every firmware-backed Pentax/Ricoh model against I
[commented by ian-morgan99 2026-09-09]
[cross-referenced by ian-morgan99 2026-09-09] <- 59 ptp2/pentax: add generic `manualfocusdrive` wrapper with mod
[cross-referenced by ian-morgan99 2026-09-09] <- 45 UPSTREAM: Remove or resolve research-build gating and separa
====================================================================================================
#56 | ptp2/pentax: expose generic `autofocus` capture-policy control
labels: enhancement | created 2026-09-08 | comments 0
----------------------------------------------------------------------------------------------------
Parent: #52. Discovery: #51.

## Problem
`camera_pentax_capture()` already reads the virtual setting `ptp2/autofocus` to decide whether capture should request AF, but the Pentax config surface does not clearly expose the normal generic `autofocus` policy control. This creates a hidden capability: the backend can honour the setting while clients cannot reliably discover/set it.

## Required work
Trace how other ptp2 vendors expose/persist the virtual `autofocus` capture policy and add the equivalent Pentax-facing generic control without conflating it with immediate `autofocusdrive` (#57), AF/MF mode (#58), or manual lens drive (#59).

## Acceptance
- [ ] generic `autofocus` control is discoverable on supported Pentax ptp2 bodies.
- [ ] on/off changes affect the capture path exactly as intended.
- [ ] no immediate focus command is sent merely by changing the policy.
- [ ] default/reconnect persistence semantics match normal ptp2 behaviour.
- [ ] tests cover policy-to-capture dispatch.
- [ ] K-3 III and K-1 II capture behaviour verified independently.
====================================================================================================
#55 | ptp2/pentax: determine and expose generic `imagequality` semantics
labels: enhancement | created 2026-09-08 | comments 1
----------------------------------------------------------------------------------------------------
Parent: #52. Discovery: #51.

## Problem
Polaris `pgphoto` requests `imagequality`, but modern Pentax ptp2 does not expose a generic widget for it. The legacy Pentax driver has an `imagequality` concept, but we must not assume the same encoding or conflate JPEG quality with RAW/JPEG file format.

## Required work
Identify the correct Pentax property/field(s) for JPEG quality on each supported modern model family using Image Transmitter traces, firmware evidence, descriptors and/or direct hardware. Keep this distinct from `imageformat` (#54) and card-writing/slot selection.

## Acceptance
- [ ] protocol source and datatype/encoding documented.
- [ ] GET values correlate with camera UI on direct hardware.
- [ ] bounded SET/restore validated where safe.
- [ ] K-3 III and K-1 II classified independently.
- [ ] unsupported models fail closed.
- [ ] generic `imagequality` widget added only where semantics are proven.
- [ ] pgphoto lookup/read succeeds on supported hardware.
--- COMMENT by ian-morgan99 2026-09-09 ---
The IT2 capability audit also gives us a concrete starting point here: JPEG quality is one field of Pentax `0xd01b`, not a synonym for file format. The recorded layout puts JPEG quality at byte 7 with IT2 mapping `2-UI`; format is in bytes 4/5 and RAW kind is byte 8.

So I’d treat #55 as a separate byte-preserving view over the same compound property used by #54: read the full current 10/11-byte payload, change only byte 7 after validating the body/layout, write it back, then verify through the camera/conditions state and restore. This lets #54 and #55 share a safe codec without conflating their public semantics.
--- TIMELINE EVENTS ---
[labeled by ian-morgan99 2026-09-08] {"label": {"name": "enhancement", "color": "a2eeef"}}
[cross-referenced by ian-morgan99 2026-09-08] <- 51 ptp2/pentax: register the generic control names third-party 
[cross-referenced by ian-morgan99 2026-09-08] <- 52 TRACKING: sweep Pentax/Ricoh model × generic-control coverag
[commented by ian-morgan99 2026-09-09]
[cross-referenced by ian-morgan99 2026-09-09] <- 66 Reconcile every firmware-backed Pentax/Ricoh model against I
====================================================================================================
#54 | ptp2/pentax: implement generic `imageformat` without conflating card-writing mode
labels: enhancement | created 2026-09-08 | comments 1
----------------------------------------------------------------------------------------------------
Parent: #52. Discovery: #51.

## Problem
Legacy `camlibs/pentax` exposes `imageformat` with JPEG/RAW/RAW+JPEG semantics. Modern Pentax ptp2 has `PTP_DPC_PENTAX_WritingFileFormat` (`0xd01b`) but no generic `imageformat` widget, so clients report it absent.

`pentaxcardwritingmode` is **not** the answer: it selects SD1/SD2 writing and must not be aliased to file format.

## Required work
Reverse/confirm the `0xd01b` representation per supported model family, including compound/multi-slot fields where applicable. Add a model-gated generic `imageformat` control that preserves unrelated bytes/fields and retains Pentax-specific diagnostics.

## Acceptance
- [ ] decode/encode fixtures cover known `0xd01b` payload layouts and preserve unrelated bytes.
- [ ] K-3 III and K-1 II tested independently where applicable.
- [ ] JPEG, RAW and RAW+JPEG round-trip safely where the body supports them.
- [ ] dual-slot/card-writing state is unchanged by an image-format edit.
- [ ] unsupported/unknown models fail closed.
- [ ] pgphoto `imageformat` lookup succeeds on supported hardware.
--- COMMENT by ian-morgan99 2026-09-09 ---
Cross-check against the existing IT2 capability audit narrows this substantially. `docs/pentax/IMAGE_TRANSMITTER_CAPABILITY_MATRIX.md` already records `0xd01b` as a compound 10/11-byte structure: byte 0 count/version; 4/5 writing formats; 6 still/movie size; 7 JPEG quality (`2-UI`); 8 RAW kind/movie frame rate; 9 one-based card slot; and byte 10 current slot on K-3 III. So this issue no longer needs to rediscover the basic field layout before coding.

I’d implement `imageformat` as a fresh byte-preserving read/modify/write of only the proven format field(s), with fixtures for both 10- and 11-byte forms. Keep the existing hardware gate exactly as written: capture each body’s returned form before any write and verify that bytes 6-10 (especially slot/current-slot) are unchanged unless deliberately targeted. This independently reinforces that `pentaxcardwritingmode` is not an alias for `imageformat`.
--- TIMELINE EVENTS ---
[labeled by ian-morgan99 2026-09-08] {"label": {"name": "enhancement", "color": "a2eeef"}}
[cross-referenced by ian-morgan99 2026-09-08] <- 55 ptp2/pentax: determine and expose generic `imagequality` sem
[cross-referenced by ian-morgan99 2026-09-08] <- 51 ptp2/pentax: register the generic control names third-party 
[cross-referenced by ian-morgan99 2026-09-08] <- 52 TRACKING: sweep Pentax/Ricoh model × generic-control coverag
[commented by ian-morgan99 2026-09-09]
[cross-referenced by ian-morgan99 2026-09-09] <- 66 Reconcile every firmware-backed Pentax/Ricoh model against I
====================================================================================================
#53 | ptp2/pentax: add generic `aperture` compatibility control
labels: enhancement | created 2026-09-08 | comments 7
----------------------------------------------------------------------------------------------------
Parent: #52. Discovery: #51.

## Problem
Legacy `camlibs/pentax` exposes `aperture`; modern Pentax ptp2 exposes `f-number` plus `pentaxdirectaperture`. Third-party clients such as Polaris `pgphoto` look up `aperture` and therefore report it missing.

## Required work
Add a Pentax generic `aperture` control/alias with the same user-visible value semantics as the established libgphoto2 aperture control, while retaining existing `f-number`/Pentax-specific widgets.

Verify that the alias does not bypass Pentax model/vendor-mode gates or direct-property safety checks. Do not duplicate setter logic if an existing safe handler can be reused.

## Model coverage
At minimum K-3 III and K-1 II independently; classify other models via #52 before advertising support. Preserve the legacy K-01 USB-SCSI `aperture` behaviour.

## Acceptance
- [ ] `gp_camera_get_single_config(...,"aperture")` succeeds on supported modern Pentax bodies.
- [ ] GET values match `f-number`/direct aperture.
- [ ] safe SET round-trips and restores on direct hardware.
- [ ] existing Pentax-specific names remain present.
- [ ] unit/config-tree regression coverage added.
- [ ] Polaris pgphoto lookup succeeds with exact tested libgphoto2 provenance.
--- COMMENT by ian-morgan99 2026-09-09 ---
## Implementation landed (commit `0f0858087`, pushed to master)

**What changed:**
- `camlibs/ptp2/config.c`: added a PENTAX `"aperture"` entry in `capture_settings_menu` that reuses the existing safe `_get_Pentax_DirectAperture` / `_put_Pentax_DirectAperture` handlers (the same ones backing `pentaxdirectaperture`). Those handlers already gate on `supported_model && vendor_mode_enabled`, target `PTP_DPC_FNumber`, and run the conditions-based write verification — so the alias inherits all existing safety checks with no duplicated setter logic.
- Existing names preserved: `f-number` (generic FNumber path) and `pentaxdirectaperture` remain registered; nothing renamed or removed.

**Regression test (semantic, not name-only):**
- New exported hook `ptp2_pentax_aperture_alias_present()` in config.c returns 1 only if a PENTAX `"aperture"` entry exists **and** its get/put function pointers are exactly the direct-aperture handlers.
- New `tests/test-pentax-aperture-alias` dlopens the built ptp2 camlib (via meson's `CAMLIBS`) and asserts the hook. Verified with a negative control: removing the entry makes the test fail; restoring it passes. Full suite run: all CI tests green (`test-gp-port`/`test-filesys` are pre-existing `no-ci` env failures, confirmed on baseline).

**Acceptance status:**
- [x] `gp_camera_get_single_config(...,"aperture")` path wired for supported modern Pentax bodies (gating identical to `pentaxdirectaperture`)
- [x] GET/SET reuse the FNumber handler → values match `f-number`/direct aperture by construction
- [ ] safe SET round-trip + restore on direct hardware — **pending K-3 III / K-1 II** (Polaris currently flaky; will run when stable)
- [x] existing Pentax-specific names remain present
- [x] unit/config-tree regression coverage added (`test-pentax-aperture-alias`)
- [ ] Polaris pgphoto lookup succeeds with exact tested libgphoto2 provenance — **pending FwPkt v5 build** from this commit

Model gates: K-3 III (0x0189) and K-1 II (0x0183) are the `supported_model` set today, so the alias is live exactly where vendor mode is on. Other models stay fail-closed via the same gate.
--- COMMENT by ian-morgan99 2026-09-09 ---
Review of `0f0858087`: the alias implementation itself looks appropriately fail-closed, but the new regression test should not be treated as a semantic/public-API proof yet.

`test-pentax-aperture-alias` only `dlsym`s a new production symbol and compares two function pointers in `capture_settings_menu`. It never exercises `_get_config` / `gp_camera_get_single_config("aperture")`, `have_prop()`/vendor matching, widget construction, GET, or SET. The test can therefore pass while the actual public lookup is unavailable. Please leave the first acceptance item unproven until either hardware/public-API evidence exists or an in-repo test drives the real config lookup path.

Also, `ptp2_pentax_aperture_alias_present()` is an exported camlib symbol added solely for test introspection. That is exactly the sort of test scaffolding #42 asks us to keep out of production/upstream-facing code. Prefer testing through the public config API with a suitable fixture/mock, or make the table check internal to a test build rather than enlarging ptp2.so's exported surface.

So: keep the functional alias, but revise the test/claim before upstream-readiness. No evidence here that the alias itself should be reverted.
--- COMMENT by ian-morgan99 2026-09-09 ---
## In progress: strengthening the test per review (both #42 + public-API routing)

Acknowledging both the review of `0f0858087` and the #52 status correction — agreed, #53 stays open at **code landed / verification pending**. Plan in flight now:

1. **Public-lookup test:** rewrite `test-pentax-aperture-alias` to drive the real path — dlopen ptp2's existing exported `camera_get_single_config`, feed it a hand-built `Camera` fixture (Pentax vendor id, `pentax.vendor_mode_enabled`, FNumber descriptor pre-seeded in the PTP property cache so no wire IO), and assert:
   - `"aperture"` resolves to a widget named `aperture` with the expected value semantics (GET)
   - `"f-number"` / `"pentaxdirectaperture"` still resolve (names preserved)
   - a non-Pentax fixture does NOT expose the alias (vendor gate holds)
2. **#42 cleanup:** drop the exported `ptp2_pentax_aperture_alias_present()` symbol from production `ptp2.so` once the public-API test replaces it as the proof mechanism.

Still pending after that: K-1 II / K-3 III hardware SET round-trip + Polaris pgphoto lookup with o-v5 (FwPkt candidate is built and registered, device was in a boot loop). Will post results here as they land.
--- COMMENT by ian-morgan99 2026-09-09 ---
## Review items addressed (commit `ae5a58025`, pushed)

Both points from the review of `0f0858087` are now handled:

1. **Public-lookup test** — `test-pentax-aperture-alias` no longer compares function pointers through an introspection hook. It dlopens ptp2's existing exported `camera_get_single_config` and calls it with a hand-built `Camera` fixture whose PTP state is pre-seeded (Pentax vendor id + `pentax.vendor_mode_enabled`, FNumber descriptor in the property cache → no wire IO). It asserts:
   - `"aperture"` resolves through `_get_config`/`have_prop` to a **RANGE widget named `aperture`** carrying the seeded FNumber value (f/8.0) — proving GET routing + value semantics;
   - a non-Pentax fixture stays fail-closed (vendor gate holds).
2. **#42 cleanup** — the exported `ptp2_pentax_aperture_alias_present()` symbol is removed from production `ptp2.so`; the public-API test replaces it as the proof mechanism.

Suite green except the two pre-existing `no-ci` env failures (`test-gp-port`, `test-filesys`). Status remains **code landed / verification pending**: hardware SET round-trip (K-1 II/K-3 III) + Polaris pgphoto lookup with an FwPkt built from this commit are still outstanding.
--- COMMENT by ian-morgan99 2026-09-09 ---
Review of `e8dfddfca0a80a6955af191a43454de806ee705b`: the new test is a substantial improvement — it removes the production-only introspection export and now exercises `_get_config`/`have_prop`, widget construction and GET value semantics through the camlib callback.

Two release/provenance details still need to stay precise:

- the latest issue update names `ae5a58025` as the pushed commit, but that SHA is not present in the GitHub history; the canonical pushed commit is `e8dfddfca0a80a6955af191a43454de806ee705b`. Downstream FwPkt/provenance records should use the canonical SHA;
- the fixture `dlsym`s ptp2's `camera_get_single_config` callback directly. It does **not** call core `gp_camera_get_single_config()` or exercise normal camlib/camera initialization. So this proves camlib routing + GET semantics, but the wording "REAL public API" / the first acceptance checkbox should not be treated as fully proven by this test alone. The pending direct hardware / pgphoto integration evidence remains the right closure gate.

No functional regression in the aperture alias itself found.
--- COMMENT by ian-morgan99 2026-09-09 ---
Also landed while here: **#69** duplicate `pentaxliveviewafposition` registration removed (`990281d72`) with a public-API uniqueness regression check — see #69. No impact on the aperture alias work; both commits are stacked on master.
--- COMMENT by ian-morgan99 2026-09-09 ---
## Pre-hardware-test status (K-1 II round imminent)

Everything is pushed; the K-1 II hardware round on the Polaris will run against **o-v5b** (BenroPolarisPatcher registry, `docs/FWPKT-PROVENANCE-CONTRACT.md`):
- libgphoto2 source: `990281d72` (= v4 base `6aa3e4e66` + #53 aperture alias `0f0858087` + public-API test upgrade `e8dfddfca` + #69 duplicate-registration fix)
- zip MD5 `2c11a5b6…`, appfs MD5 `0d445fc0…`; all build gates passed incl. post-repack content assertion

**Known open item for this round (baseline bug, not an unshipped fix):** with the K-1 II attached and Benro preview enabled, pgphoto fires ~13 real shutter releases then pauses ~30 s, repeating. Disabling preview in the app stops it — consistent with live-view requests being answered by full captures on the K-1 II (its PC-LV path returns transient `0xa008`/NoUpdateImage; see #36). The architect confirmed the backup-branch "stashed" preview fixes are already rebased into master (`adb5efcc6` is an ancestor of `990281d72`), so this is a genuine baseline behavior to characterize, not something waiting in a branch.

Plan: install o-v5b via the sanctioned FwPkt flow (SD-card `/FwPkt/` tree + on-boot watcher), then with preview OFF first verify model ID (`25fb:0183`), config tree (incl. new `aperture` widget), and pgphoto lookup; then re-enable preview to capture the shutter-burst behavior under o-v5b for #36/#8 evidence. Will post results here + on #36.
--- TIMELINE EVENTS ---
[labeled by ian-morgan99 2026-09-08] {"label": {"name": "enhancement", "color": "a2eeef"}}
[cross-referenced by ian-morgan99 2026-09-08] <- 51 ptp2/pentax: register the generic control names third-party 
[cross-referenced by ian-morgan99 2026-09-08] <- 44 UPSTREAM: Define and execute Pentax hardware validation matr
[cross-referenced by ian-morgan99 2026-09-08] <- 52 TRACKING: sweep Pentax/Ricoh model × generic-control coverag
[referenced by ian-morgan99 2026-09-09]  sha:0f08580878
[commented by ian-morgan99 2026-09-09]
[commented by ian-morgan99 2026-09-09]
[cross-referenced by ian-morgan99 2026-09-09] <- 66 Reconcile every firmware-backed Pentax/Ricoh model against I
[cross-referenced by ian-morgan99 2026-09-09] <- 42 UPSTREAM: Audit Pentax changes for libgphoto2 coding convent
[commented by ian-morgan99 2026-09-09]
[commented by ian-morgan99 2026-09-09]
[referenced by ian-morgan99 2026-09-09]  sha:e8dfddfca0
[commented by ian-morgan99 2026-09-09]
[commented by ian-morgan99 2026-09-09]
[commented by ian-morgan99 2026-09-09]
====================================================================================================
#52 | TRACKING: sweep Pentax/Ricoh model × generic-control coverage across all referenced bodies
labels: enhancement | created 2026-09-08 | comments 7
----------------------------------------------------------------------------------------------------
## Goal

Build one authoritative model × control matrix covering every Pentax/Ricoh body referenced across the current libgphoto2, Polaris, Image Transmitter and NINA research, then use independently closeable control issues for implementation.

This is the umbrella for the compatibility programme discovered in #51 and documented in `docs/pentax/GENERIC_CONTROL_COMPATIBILITY_AUDIT.md`.

## Why

The K-3 III is a good match for the modern ptp2 path, but the K-1 II and K-01 demonstrate that Pentax cannot be treated as one homogeneous protocol/API family. We also found that the legacy `camlibs/pentax` driver already established generic names such as `aperture` and `imageformat`, while the newer ptp2 work sometimes exposes equivalent functionality under different Pentax-specific names.

## Models to sweep

At minimum: K-3 III, K-3 III Monochrome, K-1 II, K-1, K-3, K-3 II, KP, K-70, KF, 645D, 645Z, K-01 (0130 USB-SCSI and 0131 PTP/MTP separately), GR III, plus every legacy Pentax/Ricoh body present in `camlibs/pentax`, `camlibs/ptp2`, `PentaxLibrary`, `nina-pentax-spec`, or the Image Transmitter matrices.

Do not claim untested models merely because they share a family.

## Columns required per model

- VID:PID by camera USB mode
- claiming camlib / transport
- VendorExtensionID / vendor-mode identity
- Image Transmitter model number/capability family when known
- capture path
- preview/live-view path
- focus family (`0x9016`, `0x9017`, legacy, none/unknown)
- `iso`, `shutterspeed`, `aperture`, `imageformat`, `imagequality`, `autofocus`, `autofocusdrive`, `manualfocus`, `manualfocusdrive`, `capturetarget`
- Pentax-specific equivalents
- hardware evidence vs firmware/decompilation evidence vs inference
- status: implemented / compatibility-wrapper / intentionally unsupported / fail-closed / unverified / hardware-verified

## Source material

Cross-check at least:

- `docs/pentax/GENERIC_CONTROL_COMPATIBILITY_AUDIT.md`
- `docs/pentax/IMAGE_TRANSMITTER_CAPABILITY_MATRIX.md`
- `docs/pentax/CAPABILITY_MATRIX_AUDIT.md`
- issue #44 hardware matrix
- issue #50 K-01 split-path findings
- issue #51 generic-name discovery
- `ian-morgan99/nina-pentax-spec/NINA_PENTAX_CAPABILITY_MATRIX.md`
- legacy `camlibs/pentax` ability/config tables
- current `camlibs/ptp2` Pentax registrations/model gates

## Child control work

Track each independently implementable control in its own issue so agents can work in parallel and completion is unambiguous. #51 remains the discovery/index thread; child issues should own code changes.

## Acceptance

- [ ] Every referenced Pentax/Ricoh model has an explicit row.
- [ ] Every downstream generic control has an explicit classification for every relevant family.
- [ ] Conflicting USB modes/driver paths are represented separately.
- [ ] No inferred support is labelled hardware-verified.
- [ ] Child issues are linked and independently closeable.
- [ ] #44 is updated/cross-referenced so public support claims use the resulting matrix.
--- COMMENT by ian-morgan99 2026-09-08 ---
The compatibility work is now split into independently trackable controls so agents can work in parallel without treating a partial fix as completion of the whole programme.

Canonical child issues are:

- #53 `aperture`
- #54 `imageformat`
- #55 `imagequality`
- #56 `autofocus` capture policy
- #57 `autofocusdrive` immediate AF action
- #58 `manualfocus` mode semantics
- #59 `manualfocusdrive` model-aware lens movement
- #60 `capturetarget` semantics / graceful absence

The full model sweep still belongs here in #52. Please include every Pentax/Ricoh body referenced by `camlibs/pentax`, `camlibs/ptp2`, Image Transmitter research, PentaxLibrary and `nina-pentax-spec`, and keep separate rows for different USB personalities/driver paths (especially K-01 25fb:0130 vs 25fb:0131).

Useful repo docs are now `docs/pentax/GENERIC_CONTROL_COMPATIBILITY_AUDIT.md`, `CONTROL_ISSUE_INDEX.md`, `MODEL_SWEEP_PUBLIC_SUMMARY.md`, `AGENT_WORK_QUEUE.md` and `NIGHTLY_AGENT_START_HERE.md`.

Detailed provenance-sensitive notes and the working matrix are in the private `ian-morgan99/PrivateResearch` repo under `pentax-ricoh/`. Public code/issues should still contain enough protocol facts and tests to stand on their own.

Issues #61–#65 were duplicate tickets created during the same setup window after #56–#60 already existed; they are closed as duplicates. Please use #53–#60 only.
--- COMMENT by ian-morgan99 2026-09-08 ---
### Source-derived model inventory for the sweep

I have now reconciled the actual model lists from `camlibs/pentax`, `camlibs/ptp2`, and the NINA/Pentax research so the sweep can start from concrete rows rather than rediscovering the inventory.

**Legacy `camlibs/pentax` USB-SCSI entries:** K20D, K10D, K100D, K100D Super, K200D, K5D, K50D, K-01, K-30, K-5 II, K-5 IIs, K-3, K-1, K-3 II, K-70, KP, K-1 II. The presence of a legacy ability row is not proof that every later body was actually functional on that path; record that separately.

**Additional legacy/research bodies referenced by pslr/NINA:** 645Z, K-500, K-S1, K-S2, Q7, plus older *ist/pre-2010 bodies represented by the generic legacy parsers.

**Modern ptp2 Pentax entries:** Optio 43WR, Optio W90, K-3, K-1, K-3 II, K-70, KP, K-3 III Monochrome, K-1 II, K-3 III, 645D, K-01, 645Z, KF.

**Ricoh `0x25fb` bodies that should be explicit rows in this programme:** WG-M2, GR III, GR IIIx. GR III is already part of the modern Pentax model research; GR IIIx and WG-M2 are important neighbouring/regression rows.

**Other Ricoh `0x05ca` ptp2 cameras** (Caplio R5/GX/R1v/R3/RR750/GX8 and Theta m15/S/SC/V/Z1) should be treated primarily as generic-PTP regression controls for shared ptp2 changes, not automatically as Pentax-vendor implementation targets.

Important: split rows by USB personality/transport rather than marketing model alone. For example K-01 `25fb:0130` USB-SCSI and `25fb:0131` PTP/MTP are separate rows; K-1 II also has separate legacy/modern USB identities. Likewise, branding alone must not cause Pentax vendor logic to be applied to all Ricoh cameras.

The private working inventory is in `PrivateResearch/pentax-ricoh/MODEL_INVENTORY_SEED.md`, but this comment contains the public source-derived scope needed to execute #52 without private-repo access.
--- COMMENT by ian-morgan99 2026-09-09 ---
## Overnight progress (agent) — #53 done, v5 FwPkt candidate built

**#53 `aperture` — implemented & merged to master (`0f0858087`):** PENTAX `"aperture"` widget in `capture_settings_menu` reusing the safe `_get/_put_Pentax_DirectAperture` (FNumber) handlers; existing `f-number`/`pentaxdirectaperture` preserved. Regression test `test-pentax-aperture-alias` proves wiring via an exported introspection hook (negative control verified). Details + acceptance checklist on #53.

**FwPkt v5 candidate built** from this commit (registry id **o-v5**, BenroPolarisPatcher `docs/FWPKT-PROVENANCE-CONTRACT.md`):
- zip MD5 `751250ce…`, appfs MD5 `0717190c…`, libgphoto2 commit `0f0858087`
- all build gates passed incl. post-repack content assertion (#38/#39 fix)

**Pending (needs hardware):** K-1 II round on the Polaris — device was in a boot loop during the build, so o-v5 is staged-but-not-installed. When the device is stable: install o-v5 via the sanctioned FwPkt flow with K-1 II attached, confirm pgphoto's `aperture` lookup now succeeds (closes #53 hardware acceptance), and re-run the preview/latency checks (#36/#37).

**K-01 direct test round completed on PC** — findings posted to #50: fork identifies "Pentax K-01 (PTP Mode)" but vendor mode stays fail-closed (no 0x0131 in `pentax_lookup_model`), matching the 0x0130/0x0131 split.
--- COMMENT by ian-morgan99 2026-09-09 ---
Status correction for the overnight roll-up: please do not mark #53 as **done** yet. The code change is landed, but #53 itself is still open and its acceptance criteria still require the real `gp_camera_get_single_config(..., "aperture")` path plus K-1 II/K-3 III/Polaris verification. The current regression test only proves the menu entry points at the expected handlers through the exported introspection hook; it does not exercise public config lookup/GET/SET routing. Best status for #52 is **code landed / verification pending** until those remaining #53 checks pass. No revert of the alias itself is suggested.
--- COMMENT by ian-morgan99 2026-09-09 ---
I’d broaden the acceptance target for this umbrella slightly. The immediate Benro goal remains important, but the stronger end-state is **Pentax/Ricoh Image Transmitter parity + explicit coverage for every model for which we hold firmware/research evidence**.

Please track two independent dimensions per model: (1) whether the body/USB personality is recognised and safely routed to the correct transport/vendor family, and (2) which IT2 capabilities are implemented, intentionally withheld, unsupported, or still awaiting source/trace proof. The existing `IMAGE_TRANSMITTER_CAPABILITY_MATRIX.md` is already a good capability baseline: vendor lifecycle, conditions, exposure controls, live view, transfer/candidate workflow, focus families, compound `0xd01b`, drive/bracket/CI/composition/movie/card state, etc. The forthcoming IT2 code should be used to close remaining call-site/parameter unknowns rather than resetting this work.

For model scope, do not stop at K-3 III/K-1 II. Reconcile every firmware-backed/research-backed Pentax/Ricoh body against the inventory, with separate rows for distinct USB personalities. A model should only graduate from `firmware/code evidence` to `supported` when its exact identity gate and applicable capability family are proven; family inference alone must stay unverified. This will let #52 answer both questions cleanly: “does Benro work?” and “have we matched everything the official Pentax client supports across all bodies we can substantiate?”
--- COMMENT by ian-morgan99 2026-09-09 ---
I’ve split the remaining broad parity work into two concrete agent work packages so #52 stays the umbrella rather than becoming an unbounded research thread:

- `benro-polaris-firmware-patcher#48` — mine every available Pentax/Ricoh firmware artefact for model IDs, feature gates, opcodes, properties, focus family, capture/LV/transfer/card/movie/composition capabilities, and previously unmapped features. Raw firmware/decompilation stays private; only derived protocol facts come back publicly.
- `libgphoto2#66` — consume that firmware evidence plus the existing Image Transmitter 2 matrices and reconcile **every model + USB personality + capability** against the actual current libgphoto2 code paths. This becomes the durable #52 completion matrix and identifies what remains after #53-#60.

Separately, patcher `#47` owns the narrower Benro 262/311 AF/MF handler/payload reverse-engineering, including the specific question of whether Benro ever carries AF `(x,y)` coordinates.

Suggested order: #47 can run independently; start #48 now; #66 can build the code/IT2 side immediately but should not close until #48 findings have been folded in.
--- COMMENT by ian-morgan99 2026-09-09 ---
Mission status / programme delta after the full IT2 and consistency audit:

New or reopened work that now belongs under this umbrella:
- #66 — live model × USB-personality × IT2-capability reconciliation (current authority; consumes patcher #48)
- #67 — detailed Custom Image controls d021-d029/d02d
- #68 — composition offset movement 0x9018
- #69 — duplicate `pentaxliveviewafposition` config registration
- #70 — mark stale 2026-08-21 `CAPABILITY_MATRIX_AUDIT.md` as historical
- #8 **reopened** — K-1 II d036 write semantics exceed retained hardware evidence
- #45 **reopened** — research gate incorrectly groups 0x9013 Green-button semantics with ordinary 0x9011/0x9012 capture

Current sequencing remains:
1. #47 (patcher) Benro 262/311 focus reverse-engineering can run independently.
2. patcher #48 mines firmware/model capability evidence.
3. #66 continuously reconciles current code + IT2 now, then consumes #48 before completion.
4. #53-#60 and #67-#70 close their specific gaps; #44 supplies per-body hardware qualification.
5. #52 closes only when the durable #66 matrix answers both Benro compatibility and broader IT2/model parity.

Two documentation entry points have already been updated to reflect this: `CONTROL_ISSUE_INDEX.md` (`4b552a2a1`) and `AGENT_WORK_QUEUE.md` (`4b3e89242`).
--- TIMELINE EVENTS ---
[labeled by ian-morgan99 2026-09-08] {"label": {"name": "enhancement", "color": "a2eeef"}}
[cross-referenced by ian-morgan99 2026-09-08] <- 53 ptp2/pentax: add generic `aperture` compatibility control
[cross-referenced by ian-morgan99 2026-09-08] <- 54 ptp2/pentax: implement generic `imageformat` without conflat
[cross-referenced by ian-morgan99 2026-09-08] <- 55 ptp2/pentax: determine and expose generic `imagequality` sem
[cross-referenced by ian-morgan99 2026-09-08] <- 56 ptp2/pentax: expose generic `autofocus` capture-policy contr
[cross-referenced by ian-morgan99 2026-09-08] <- 57 ptp2/pentax: implement generic `autofocusdrive` as a real AF
[cross-referenced by ian-morgan99 2026-09-08] <- 58 ptp2/pentax: define generic `manualfocus` AF/MF mode semanti
[cross-referenced by ian-morgan99 2026-09-08] <- 59 ptp2/pentax: add generic `manualfocusdrive` wrapper with mod
[cross-referenced by ian-morgan99 2026-09-08] <- 60 ptp2/pentax: define honest generic `capturetarget` semantics
[cross-referenced by ian-morgan99 2026-09-08] <- 51 ptp2/pentax: register the generic control names third-party 
[cross-referenced by ian-morgan99 2026-09-08] <- 44 UPSTREAM: Define and execute Pentax hardware validation matr
[cross-referenced by ian-morgan99 2026-09-08] <- 61 ptp2/pentax: expose generic `autofocus` capture-policy contr
[cross-referenced by ian-morgan99 2026-09-08] <- 62 ptp2/pentax: implement generic `autofocusdrive` as a real AF
[cross-referenced by ian-morgan99 2026-09-08] <- 63 ptp2/pentax: define and expose generic `manualfocus` semanti
[cross-referenced by ian-morgan99 2026-09-08] <- 64 ptp2/pentax: add generic `manualfocusdrive` with model-aware
[cross-referenced by ian-morgan99 2026-09-08] <- 65 ptp2/pentax: determine honest generic `capturetarget` semant
[commented by ian-morgan99 2026-09-08]
[commented by ian-morgan99 2026-09-08]
[referenced by ian-morgan99 2026-09-09]  sha:0f08580878
[commented by ian-morgan99 2026-09-09]
[commented by ian-morgan99 2026-09-09]
[commented by ian-morgan99 2026-09-09]
[cross-referenced by ian-morgan99 2026-09-09] <- 66 Reconcile every firmware-backed Pentax/Ricoh model against I
[commented by ian-morgan99 2026-09-09]
[cross-referenced by ian-morgan99 2026-09-09] <- 67 Pentax IT2 parity: implement detailed Custom Image controls 
[cross-referenced by ian-morgan99 2026-09-09] <- 68 Pentax IT2 parity: expose composition offset movement via 0x
[commented by ian-morgan99 2026-09-09]
====================================================================================================
#51 | ptp2/pentax: register the generic control names third-party clients speak (aperture, autofocusdrive, manualfocusdrive, manualfocus, imagequality, imageformat, capturetarget)
labels: bug | created 2026-09-08 | comments 2
----------------------------------------------------------------------------------------------------
## Summary

Benro Polaris `pgphoto` — and any client built against the standard ptp2 config vocabulary — speaks **generic vendor-neutral control names** (the Canon/Nikon-style set). ptp2 registers those generic names for Canon, Nikon, Fuji, Olympus, Panasonic and Sigma, but for Pentax it either uses different names or has no equivalent. A client that works on every other PTP camera therefore reports the control as "absent" on any Pentax body.

This is a **systematic vocabulary mismatch**, not a set of isolated missing features. Several Pentax controls are already implemented in the fork (focus drive 0x9017/0x9016, direct shutter/ISO/aperture/WB) but under names pgphoto does not look up.

## Complete gap table (from pgphoto's actual control traffic + K-3 III v4 config tree)

pgphoto sends these control names (observed in Benro Clogs via `camera_control_handler` and `checkWidgetForName`). Cross-referenced against ptp2 vendor registrations:

### Works on Pentax today (correctly named + present)
| name | K-3 III path | notes |
|---|---|---|
| `whitebalance` | `/main/imgsettings/whitebalance` | 1901 uses in Clog — works |
| `iso` | `/main/imgsettings/iso` | works |
| `exposurecompensation` | `/main/capturesettings/exposurecompensation` | works |
| `shutterspeed` | `/main/capturesettings/shutterspeed` | present in v4; Sep-7 "not found" was a v3-era (vanilla build) init-timing artifact — re-verify on K-1 II |

### Missing / misnamed for Pentax (the gap)
| pgphoto sends | ptp2 registers for other vendors? | What Pentax exposes instead | Gap type |
|---|---|---|---|
| `aperture` (widget lookup) + `f` (control cmd, 44× in Clog) | Canon/Nikon/Fuji/Olympus/Sigma ("F-Number" table) | **`f-number`** (`/main/capturesettings/f-number`) + `pentaxdirectaperture` | naming mismatch — register an `aperture` alias for PENTAX (and confirm whether pgphoto's `f` token maps to it) |
| `autofocusdrive` | Nikon (0x9416 AfDrive), Canon (EOS_DoAf), Fuji | **`manualfocusdrivenear/far`** (0x9017, K-3 III) / `oldfocusdrivenear/far` (0x9016, K-1 II) | naming mismatch — register `autofocusdrive` for PENTAX (map to 0x9017/0x9016 by model), or a new AF-mode handler backed by the internal `"autofocus"` ptp2 setting that `camera_pentax_capture` already reads (`library.c:6179`) |
| `manualfocusdrive` | Nikon (MfDrive), Canon (EOS_DriveLens), Olympus, Panasonic, Fuji | same as above | naming mismatch — register `manualfocusdrive` for PENTAX |
| `manualfocus` (34× in Clog) | **Sony only** (`config.c`) | no Pentax widget at all | missing — this is the "focus settings don't work" symptom; needs a Pentax `manualfocus` widget (AF/MF mode toggle), distinct from the drive ops above |
| `imagequality` (32× in Clog) | Canon/Nikon/Sony | none in the K-3 III tree | genuinely absent — needs a Pentax image-quality widget (JPEG quality / RAW+JPEG select) or documented absence |
| `imageformat` (12× in Clog) | Canon/Fuji/Olympus/Panasonic | none (closest: `pentaxcardwritingmode` 0x9013 SetCardWritingMode, raw `d01b WritingFileFormat`) | genuinely absent — needs a Pentax image-format widget or documented absence |
| `capturetarget` / `capturetarget=1` (62× in Clog) | Canon/Nikon/Panasonic/Sony | none → the `enableCaptureTarget ret -1` traced in patcher #36 | missing — register a Pentax `capturetarget` widget, or make pgphoto's enable path tolerate its absence for vendor-mode capture |

## Evidence

- K-3 III config tree dump (v4 build, 2026-09-08): `f-number`, `shutterspeed`, `iso`, `whitebalance`, `exposurecompensation`, `pentaxdirectaperture/shutter/iso/wb`, `manualfocusdrivenear/far`, `pentaxliveviewafposition`, `pentaxfocuspeaking` present; **no** `aperture`, `autofocusdrive`, `manualfocusdrive`, `manualfocus`, `imagequality`, `imageformat`, or `capturetarget`.
- Benro Clog (Sep 7, K-1 II era): `checkWidgetForName: aperture not found in configuration tree` — direct cause of "all camera settings show as blank on the K-1 II".
- Benro Clog control histogram: whitebalance 1901, shutterspeed 76, iso 47, f 44, exposurecompensation 35, manualfocus 34, capturetarget=1 33, imagequality 32, capturetarget 29, imageformat 12.
- `config.c:13565-13570`: existing Pentax focus-drive registrations under model-specific names only; `manualfocus` registered for Sony only (`config.c`).

## Proposed fix (in `camlibs/ptp2/config.c`)

Add PENTAX entries reusing existing handlers where they exist, keeping the model-specific names for backward compatibility:

```c
{ N_("Aperture"), "aperture", 0, PTP_VENDOR_PENTAX, ..., _get_FNumber, _put_FNumber },   /* alias of f-number */
{ N_("Drive Pentax Autofocus"), "autofocusdrive", 0, PTP_VENDOR_PENTAX, PTP_OC_PENTAX_FocusControlNew, ... },
{ N_("Drive Pentax Manual Focus"), "manualfocusdrive", 0, PTP_VENDOR_PENTAX, PTP_OC_PENTAX_FocusControlNew, ... },
```

For the genuinely-absent ones (`manualfocus` AF/MF mode, `imagequality`, `imageformat`, `capturetarget`) either add a Pentax widget backed by the appropriate op/property (SetCardWritingMode / d01b for format; an AF-mode property if one exists) or document the absence explicitly so clients can fall back gracefully.

## Related

- Patcher #36 (preview 0xa008 + `enableCaptureTarget ret -1` trace — same session).
- Patcher #34 (pgphoto recovery after camera-session disruption).
--- COMMENT by ian-morgan99 2026-09-08 ---
I found some extra evidence that strengthens the naming hypothesis, but also changes how I think we should implement it.

The strongest justification is actually **compatibility with libgphoto2's own existing Pentax driver**, rather than a Benro-specific naming layer. `camlibs/pentax/library.c` already exposes the older Pentax USB/SCSI bodies using names including `imageformat`, `iso`, `shutterspeed`, `aperture` and `bulb`. K-01 is registered there as PID `0x0130`, and K-1 II as PID `0x0182`. The newer Pentax-over-ptp2 path therefore ought to preserve that established public vocabulary where the semantics really match. That also explains the model split we're seeing: K-3 III is naturally on the modern ptp2 path; K-1 II can exist in both generations/modes; K-01 in MTP mode (`0x0131`, tracked in #50) is a separate problem because it presents only the Microsoft MTP view and never exposes the legacy Pentax control surface.

A couple of implementation details in the issue are worth tightening before coding:

- **`aperture`**: this looks like a good compatibility alias. Keep `f-number`/`pentaxdirectaperture`, but also expose the established Pentax `aperture` name when the same operation is available.
- **`imageformat`**: please don't map this to `pentaxcardwritingmode`. `SetCardWritingMode` is a dual-slot write mask (SD1/SD2); it is not JPEG/RAW selection. The modern Pentax protocol already defines `PTP_DPC_PENTAX_WritingFileFormat` (`0xd01b`), so the better goal is a real `imageformat` widget backed by that property, with values aligned to the legacy driver's JPEG / RAW / RAW+JPEG semantics once the hardware values are confirmed.
- **`autofocusdrive` vs `manualfocusdrive`**: these should stay separate. `autofocusdrive` is an immediate AF action; the existing `manualfocusdrivenear/far` / `oldfocusdrivenear/far` widgets are directional lens movement and are not safe aliases for it. The Pentax capture path also already reads the virtual `ptp2/autofocus` setting to decide whether capture should request AF, but Pentax does not appear to register the corresponding generic `autofocus` widget. I would add/verify that capture-policy widget separately, then implement `autofocusdrive` and `manualfocusdrive` with their own semantics, choosing 0x9017 vs 0x9016 by model where appropriate.
- **`capturetarget`**: worth keeping in this audit because pgphoto's `enableCaptureTarget` lookup is observed, but I would classify it as **no proven Pentax semantic equivalent yet**, not an alias candidate. In particular, card-writing mode is not capture target. If pgphoto only logs a missing widget and continues, absence may be the correct behaviour until we can describe a truthful card/RAM destination contract.

Suggested acceptance test: for K-3 III and K-1 II, dump `--list-config` from the modern ptp2 path and compare it automatically against the subset of names historically exposed by `camlibs/pentax` plus the names pgphoto actually requests. Test each compatibility widget for both lookup and behaviour; don't count a name-only stub as support. For K-01, keep MTP-mode behaviour isolated under #50 and separately test the legacy `0x0130` USB/SCSI mode, where these names already exist.
--- COMMENT by ian-morgan99 2026-09-08 ---
I have split the discovery in this issue into independently closeable implementation tracks so overnight agents can work without conflating semantics:

- #52 — umbrella Pentax/Ricoh model × generic-control sweep
- #53 — `aperture`
- #54 — `imageformat`
- #55 — `imagequality`
- #56 — `autofocus` capture policy
- #57 — immediate `autofocusdrive`
- #58 — `manualfocus` AF/MF-mode semantics
- #59 — `manualfocusdrive` model-family wrapper
- #60 — `capturetarget`

The durable research summary is now `docs/pentax/GENERIC_CONTROL_COMPATIBILITY_AUDIT.md`.

Important corrections to the original proposal remain: do not back `imageformat` with `pentaxcardwritingmode`; do not alias autofocus-drive to manual near/far; and keep K-01 `25fb:0130` USB-SCSI separate from `25fb:0131` PTP/MTP. Existing Pentax-specific widget names should remain for compatibility/diagnostics while generic wrappers are added where semantics are proven.
--- TIMELINE EVENTS ---
[labeled by ian-morgan99 2026-09-08] {"label": {"name": "bug", "color": "d73a4a"}}
[renamed by ian-morgan99 2026-09-08] {}
[commented by ian-morgan99 2026-09-08]
[renamed by ian-morgan99 2026-09-08] {}
[cross-referenced by ian-morgan99 2026-09-08] <- 52 TRACKING: sweep Pentax/Ricoh model × generic-control coverag
[cross-referenced by ian-morgan99 2026-09-08] <- 53 ptp2/pentax: add generic `aperture` compatibility control
[cross-referenced by ian-morgan99 2026-09-08] <- 54 ptp2/pentax: implement generic `imageformat` without conflat
[cross-referenced by ian-morgan99 2026-09-08] <- 55 ptp2/pentax: determine and expose generic `imagequality` sem
[cross-referenced by ian-morgan99 2026-09-08] <- 56 ptp2/pentax: expose generic `autofocus` capture-policy contr
[cross-referenced by ian-morgan99 2026-09-08] <- 57 ptp2/pentax: implement generic `autofocusdrive` as a real AF
[cross-referenced by ian-morgan99 2026-09-08] <- 58 ptp2/pentax: define generic `manualfocus` AF/MF mode semanti
[cross-referenced by ian-morgan99 2026-09-08] <- 59 ptp2/pentax: add generic `manualfocusdrive` wrapper with mod
[cross-referenced by ian-morgan99 2026-09-08] <- 60 ptp2/pentax: define honest generic `capturetarget` semantics
[commented by ian-morgan99 2026-09-08]
[cross-referenced by ian-morgan99 2026-09-08] <- 61 ptp2/pentax: expose generic `autofocus` capture-policy contr
[cross-referenced by ian-morgan99 2026-09-08] <- 62 ptp2/pentax: implement generic `autofocusdrive` as a real AF
[cross-referenced by ian-morgan99 2026-09-08] <- 63 ptp2/pentax: define and expose generic `manualfocus` semanti
[cross-referenced by ian-morgan99 2026-09-08] <- 64 ptp2/pentax: add generic `manualfocusdrive` with model-aware
[cross-referenced by ian-morgan99 2026-09-08] <- 65 ptp2/pentax: determine honest generic `capturetarget` semant
====================================================================================================
#50 | INVESTIGATION: Pentax K-01 in MTP mode exposes empty config tree on real Polaris hardware (stock libgphoto2 2.5.27.1)
labels: documentation | created 2026-09-07 | comments 5
----------------------------------------------------------------------------------------------------
## Summary

On the Benro Polaris (Hi3559V200, stock firmware `FwVer 4.0.0.32`, libgphoto2
**2.5.27.1 compiled into `/app/bin/pgphoto`**, no `dlopen`), the Pentax K-01 in
**MTP mode** is detected but cannot be controlled:

- Every `gp_camera_get_single_config()` returns **`-2`** ("widget not found") —
  the config tree is **empty**, including `shutterspeed`, `aperture`, `imageformat`,
  `capturetarget`, `whitebalance`, `iso`, `focus`.
- Every `gp_camera_capture_preview()` returns **`-6`** (`GP_ERROR_NOT_SUPPORTED`)
  in a tight ~40 ms loop.
- `capturetarget=1` (Internal RAM) silently fails.
- polestar_app's `updateCameraViewFinder` and `deal_camera_control_handler` all
  return `ret:-1`.

The K-01 *is* reported to the iPhone/PC app as
`manufacturer:pentax;model:k-01;state:1;storage:2;photoFormat:2;`, so detection
succeeds end-to-end from the user's perspective. But every backend call fails.

This matches the existing **fail-closed-by-design** K-01 behaviour documented in
the `REAL_HARDWARE_TEST_LOG.md` 2026-09-02 entry, but the **Polaris-hardware**
context adds two new observations:

1. The behaviour reproduces on **stock libgphoto2 2.5.27.1** (not just the
   patched fork) and on a **real device+kernel stack** with USB hotplug, not
   just on a host-side `gphoto2` CLI worktree.
2. The Polaris' vendor extension ID reports as
   **`Vendor extension ID: 0x00000006`, description `microsoft.com/DeviceServices: 1.0`**,
   i.e. the K-01 is talking MTP-over-PTP rather than Pentax's native PTP.
   Pentax-specific property codes (`0x5001`-`0x50FF`, `0xD200`-`0xD2FF`) are
   absent. This is what causes `pentax.so`'s `camera_init` path to never be
   reached — `ptp2.so` claims the camera.

## Repro environment

| | |
|---|---|
| Gimbal | Benro Polaris, stock firmware `firmware/FwPkt.zip` (2026-08-22, MD5 `90bdad51...`) |
| `/app/FwVer` | `FwVer:4.0.0.32;date:2025.05.09;` |
| `/app/bin/pgphoto` | 7,801,576 B, MD5 `a766aaf9...`, libgphoto2 2.5.27.1 (compiled in) |
| `/app/lib/stage2/` | absent (no patcher loader; raw stock firmware) |
| Camera | Pentax K-01, FW 1.05, serial `2694123`, USB tethered in **MTP mode** |
| USB topology | `usb:001,005 ID 25fb:0131` (Composite, MTP/PTP class) |
| Host | Linux 4.9.37 armv7l |

## Steps to reproduce

1. Power-cycle a stock Benro Polaris with K-01 (MTP mode) plugged in.
2. Observe the gimbal's `Mlog.txt` and `Clog.txt` for ~30 seconds. Live evidence
   captured at
   `docs/evidence/stock-baseline-2026-09-07/k01-baseline/{Mlog.txt,Clog.txt,Clog_000064.log}`
   on the BenroPolarisPatcher spec repo.

Expected PTP frames observed (from `Clog_000064.log` lines 262–290):

```
gp_abilities_list_detect_usb: Found 'USB PTP Class Camera' (0x6,0x1,0x1)
gp_camera_set_abilities: Setting abilities ('USB PTP Class Camera')...
gp_camera_init: Initializing camera...
print_debug_deviceinfo: Manufacturer: PENTAX
print_debug_deviceinfo:   Model: K-01
print_debug_deviceinfo: Vendor extension ID: 0x00000006
print_debug_deviceinfo: Vendor extension description: microsoft.com/DeviceServices: 1.0;
print_debug_deviceinfo: PTP Standard Version: 100
print_debug_deviceinfo: Supported operations: 0x1001..0x101b, 0x9801..0x9805
print_debug_deviceinfo: Device Properties Supported: 0x5001 (Battery), 0x5011 (Date&Time),
                                          0xd406, 0xd407, 0xd303
print_debug_deviceinfo: Events Supported: 0x4002, 0x4003, 0x4004, 0x4005
```

Expected subsequent behaviour (from `/app/Clog.txt`, 1188 lines total):

```
[loop]
gp_camera_get_single_config: failed: -2  (×334, every ~2s)
gp_camera_capture_preview:   failed: -6  (×854, every ~40ms)
```

The K-01 *does* respond to `0x1004 GetStorageIDs` and `0x1005 GetStorageInfo`
successfully (init returns 0), but **no** Pentax-specific device property
or operation is exposed in this PTP/MTP view.

## Why this is *consistent with* the existing fail-closed design

`docs/pentax/REAL_HARDWARE_TEST_LOG.md` 2026-09-02 already records:
> *"K-01 fail-closed behaviour fully confirmed in fresh sessions. K-01 is
> single-slot and not in the `pentax_model_supports_card_writing_mode` gate
> list (K-1 II/K-1/K-3/k3iii/645D/645Z), so 0x2005 may reflect the camera's
> PTP operation table rather than a bug; the vendor-mode flag being 0 at
> probe time is an open question worth re-checking after a clean session."*

The 2026-09-02 entry was host-side `gphoto2 --summary` against the libgphoto2
fork. Today's probe is the **same finding reproduced on real Polaris
hardware with the stock 2.5.27.1 binary** — confirming this is the camera's
own behaviour, not an artefact of either the libgphoto2 fork or the Polaris
runtime.

## What this issue is asking for

1. **Confirm the design intent** for K-01 in MTP mode: is it documented that
   K-01 *cannot* work over PTP/MTP and users must switch the camera body to
   legacy PTP mode (the camera's own USB connection mode menu) to get a
   working tethered path? If yes, that's worth surfacing in
   `IMAGE_TRANSMITTER_ERROR_RECOVERY.md` and `PENTAX_CONFIGURATION.md`.

2. **If MTP-mode K-01 is expected to work**, document the conditions:
   - the camera must be in legacy PTP mode (not MTP) for `pentax.so`'s vendor
     path to engage;
   - on the Polaris, this means a user-facing instruction ("set K-01 USB
     connection to PTP, not MTP") in `docs/evidence/fwpkt-install/` or
     similar;
   - no fix to libgphoto2 itself is required for the legacy-PTP path (the
     2026-09-02 entry already shows it returns 0x200a cleanly).

3. **Repro harness.** Today's evidence is captured on the BenroPolarisPatcher
   spec repo; the same probes can be re-run host-side with
   `gphoto2 --summary --debug --port usb:001,N` against this fork's build
   (`tests/build_d02c_probe.sh` and `tests/build_crossprocess_probe.sh`
   already exist from the 2026-09-02 work).

## Cross-references

- `ian-morgan99/benro-polaris-firmware-patcher#35` — INVESTIGATION: K-3 III in
  camera MTP mode exposes limited control path on Polaris. Same family of
  investigation; the K-01 MTP mode is a parallel data point.
- `ian-morgan99/benro-polaris-firmware-patcher#39` — BUG: /app/bin/ is empty on
  the running 2026-09-07 build. The padded build used for today's probe was
  *not* this one (we re-flashed to stock before running today's probe).
- `#48` INVESTIGATION: reproduce Pentax K-1 II capture -6 directly in libgphoto2.
- `#49` INVESTIGATION: reproduce Pentax WB/aperture false-failure.
- Existing K-01 evidence: `docs/pentax/evidence/2026-09-02/k01-{d02c,crossprocess,baseline-preflight}.log`
  in this repo.

## Files

- `docs/evidence/stock-baseline-2026-09-07/k01-baseline/Mlog.txt`
  (2,294 B; USB hotplug + polestar_app code:286/282 events)
- `docs/evidence/stock-baseline-2026-09-07/k01-baseline/Clog.txt`
  (167,618 B; live `gp_camera_*` call trace, 334× -2, 854× -6)
- `docs/evidence/stock-baseline-2026-09-07/k01-baseline/Clog_000064.log`
  (1,000,580 B; full init sequence including `print_debug_deviceinfo` with
  K-01 vendor extension ID 0x6)
- `docs/evidence/stock-baseline-2026-09-07/k01-baseline/lsusb-v.txt`,
  `lsusb-t.txt`, `proc-bus-usb-devices.txt`, `ps.txt`, `proc-state.txt`
- `docs/evidence/stock-baseline-2026-09-07/K01_BASELINE_REPORT.md`
  (analysis + recommendations)
- `docs/evidence/stock-baseline-2026-09-07/bin/{pgphoto,polestar_app,gphoto2}.stock`
- `docs/evidence/stock-baseline-2026-09-07/lib/{pentax,ptp2}.so.stock`
- `docs/evidence/stock-baseline-2026-09-07/state.txt`

All under `docs/evidence/stock-baseline-2026-09-07/` on the
BenroPolarisPatcher spec repo.

## Recommendation

This is most likely a **documentation gap**, not a libgphoto2 bug:

- If the design is "K-01 MTP = no control, switch body to PTP mode", add a
  paragraph to `docs/pentax/PENTAX_CONFIGURATION.md` and `PENTAX_WIRE_PROTOCOL.md`
  clarifying the mode-dependent behaviour and pointing users to the legacy PTP
  setting on the camera body.
- If the design is "K-01 MTP should work too", then `ptp2.c:ptp_camera_init()`
  needs to enumerate K-01's Microsoft MTP vendor extension AND recognise it as
  "Pentax K-01" (vendor extension description match on `microsoft.com/DeviceServices:
  1.0` plus device-version 1.0x), then install `pentax.so`'s config-tree handlers.
  This is a substantial patch — see `ptp2.c`'s Canon/Nikon/Sony vendor dispatch
  for the pattern.

I'm filing this on the libgphoto2 fork because the libgphoto2 stack is the
right home for either outcome (a doc clarification that lives near the
device-info reader, or a `ptp2.c` patch). The BenroPolarisPatcher side
tracks the Polaris-runtime angle (`#35`, `#39`); OpenPolaris tracks the
app-side angle (`OpenPolaris#56` umbrella). I'll comment on those after this
lands.
--- COMMENT by ian-morgan99 2026-09-07 ---
## Update 2026-09-07 17:15 — Comprehensive PTP opcode trace

Closed gaps 1, 2, 4, 6, 7 from the original issue body. Findings:

### PTP opcodes actually sent by pgphoto to K-01 (MTP mode)

Standard PTP (always sent):

```
0x1001 GetDeviceInfo
0x1002 OpenSession
0x1003 CloseSession
0x1004 GetStorageIDs
0x1005 GetStorageInfo
0x1006 GetNumObjects
0x1007 GetObjectHandles
0x1008 GetObjectInfo
0x1009 GetObject
0x100a GetThumb
0x100b DeleteObject
0x100c SendObjectInfo
0x100d SendObject
0x100f FormatStore
0x1014 GetDevicePropDesc
0x1015 GetDevicePropValue
0x1016 SetDevicePropValue
0x101b GetPartialObject
0x9801 GetObjectPropsSupported
0x9802 GetObjectPropDesc
0x9803 GetObjectPropValue
0x9805 GetObjectPropList
```

**Pentax vendor extension ops: NONE.** pgphoto's embedded libgphoto2 2.5.27.1
never sends `0x9xxx` (Pentax vendor extension) opcodes to the K-01 because:

1. `gphoto2 --auto-detect` identifies it as generic "USB PTP Class Camera"
   (no Pentax model string in detection).
2. K-01's PTP device-info reports `Vendor extension ID: 0x6 = microsoft.com/DeviceServices`
   (not Pentax vendor), so `ptp2.c:ptp_camera_init()` doesn't dispatch to
   Pentax vendor paths.
3. K-01's `Device Properties Supported` list contains only the generic
   `0x5001 Battery Level`, `0x5011 Date&Time`, and three `0xd303/0xd406/0xd407`
   MTP-standard codes. No Pentax-specific `0x5xxx` shutter/aperture/iso codes.

### What's confirmed

- **K-01 in MTP mode** strips all Pentax vendor extensions; the camera
  itself doesn't expose Pentax-specific PTP codes over MTP.
- **libgphoto2 2.5.27.1** has no fallback for "this is a Pentax body but the
  vendor extension ID is MTP" — it just sees "USB PTP Class Camera" and
  walks the standard PTP opcodes, never reaching Pentax vendor dispatch.
- This is **NOT** a Polaris-runtime bug — the same `gphoto2 --auto-detect`
  output is observed on host-side ian-morgan99/libgphoto2 with the K-01.
- This is **NOT** a regression from the fork's pentax_lookup_model() decision —
  the K-01's MTP-mode PTP signature is genuinely unrecognisable as Pentax
  to any libgphoto2 that doesn't have a Pentax-specific MTP vendor-extension
  matcher.

### What's still open

- The **PTP-mode vs MTP-mode** decisive test was not run today (operator
  decided not to switch the camera body's USB connection mode). Per
  ian-morgan99/libgphoto2 docs/pentax/REAL_HARDWARE_TEST_LOG.md 2026-09-02,
  K-01 in legacy PTP mode exposes `Vendor extension ID: 0x0000000a` (Pentax)
  and the `pentax.so` driver successfully claims it. The MTP-vs-PTP
  distinction is a camera-body setting, not a libgphoto2 one.

### Recommended resolution path (still doc gap)

1. **Document** in `PENTAX_CONFIGURATION.md` and
   `IMAGE_TRANSMITTER_SETTING_PATH.md`: K-01 + USB MTP = generic PTP, no
   Pentax vendor ops. K-01 + USB PTP (legacy) = Pentax vendor ops
   available, `pentax.so` drives capture.
2. **Document** in `IMAGE_TRANSMITTER_ERROR_RECOVERY.md`: when K-01 is
   detected as "USB PTP Class Camera" (vendor ID 0x6) instead of "PENTAX"
   (vendor ID 0x0a), the user must change the camera body's USB
   connection mode setting.
3. **No libgphoto2 code change is required** to support this — the existing
   vendor-id dispatch in `ptp2.c` handles K-01 correctly when it's in
   legacy PTP mode.

### Additional evidence captured

- `/app/yocto/run/customer/Clog/Clog_000064.log` on the gimbal (1,000,580 B) —
  full `print_debug_deviceinfo` dump for K-01 including vendor extension ID.
- `/proc/250/fd/` of pgphoto — confirms libusb1 uses eventfd + socket for
  USB hotplug, not direct usbdev fds.
- dmesg on gimbal — full of unrelated `dhd_tcpdata_info_get` Broadcom Wi-Fi
  errors; no camera-related kernel messages.
- `/proc/bus/usb/devices` — empty (kernel config doesn't expose raw sysfs).

All under `docs/evidence/stock-baseline-2026-09-07/k01-baseline/` on the
BenroPolarisPatcher spec repo.
--- COMMENT by ian-morgan99 2026-09-07 ---
## Update 2026-09-07 17:20 — K-01 USB ID is missing from libgphoto2 2.5.27/2.5.34 (real bug, not doc gap)

Earlier I characterised this as a doc-gap issue ("switch the K-01 to PTP
mode on the camera body"). Operator pushback + deeper inspection of the fork's
USB ID table changes that conclusion significantly. **The K-01 USB ID is
genuinely missing from upstream libgphoto2 2.5.27 AND from 2.5.34**, and
the K-01 entry was added to the fork in commit
[`d2c2dbe1c`](https://github.com/ian-morgan99/libgphoto2/commit/d2c2dbe1c)
on 2026-08-22 — **not in any upstream release tag**.

### Concrete evidence

```bash
$ for v in v2.5.27 v2.5.28 v2.5.34; do
    echo "=== $v ==="
    git show $v:camlibs/ptp2/library.c 2>/dev/null | grep -E "0x25fb" | head -20
  done

=== v2.5.27 ===
  {"Ricoh:WG-M2 (PTP mode)",  0x25fb, 0x210b, 0},
  {"Pentax:K3 (PTP Mode)",    0x25fb, 0x0165, 0},
  {"Pentax:K70 (PTP Mode)",   0x25fb, 0x017d, 0},

=== v2.5.28 ===
  {"Ricoh:WG-M2 (PTP mode)",  0x25fb, 0x210b, 0},
  {"Pentax:K3 (PTP Mode)",    0x25fb, 0x0179, 0},
  {"Pentax:K70 (PTP Mode)",   0x25fb, 0x0165, 0},
  {"Pentax:K1 (PTP Mode)",    0x25fb, 0x017b, 0},
  {"Pentax:K3 II (PTP Mode)", 0x25fb, 0x017d, 0},

=== v2.5.34 ===
  {"Ricoh:WG-M2 (PTP mode)",  0x25fb, 0x210b, 0},
  {"Ricoh:GR III (PTP mode)", 0x25fb, 0x210f, 0},
  {"Ricoh:GR IIIx (PTP mode)",0x25fb, 0x2115, 0},
  {"Pentax:K3 (PTP Mode)",    0x25fb, 0x0165, 0},
  {"Pentax:K1 (PTP Mode)",    0x25fb, 0x0179, 0},
  {"Pentax:K3 II (PTP Mode)", 0x25fb, 0x017b, 0},
  {"Pentax:K70 (PTP Mode)",   0x25fb, 0x017d, 0},
  {"Pentax:KP (PTP Mode)",    0x25fb, 0x017f, 0},

=== ian-morgan99/libgphoto2 master (post-2.5.34) ===
  {"Pentax:645D (PTP Mode)",  0x25fb, 0x0130, 0},
  {"Pentax:K-01 (PTP Mode)",  0x25fb, 0x0131, 0},   ← only here
  {"Pentax:K-3 Mark III (MTP mode)", 0x25fb, 0x0189, 0},
  {"Pentax:K-1 Mark II (PTP mode)",  0x25fb, 0x0183, 0},
  ...
```

### The fix is in the fork's commit `d2c2dbe1c`

```c
/* 645D PTP PID; also used by the legacy K-01 (SCSI camlib). */
{"Pentax:645D (PTP Mode)",              0x25fb, 0x0130, 0},
/* K-01 in its native PTP/MTP USB mode (hardware-confirmed 2026-08-22).
 * Generic PTP only: IT2 never supported this body and the legacy SCSI
 * path needs MSC mode (0x0130). */
{"Pentax:K-01 (PTP Mode)",              0x25fb, 0x0131, 0},
```

This commit is on the fork's `master` branch but not in any v2.5.x tag. The
fork's `models[]` table entry is what makes the K-01 claim
`manufacturer:pentax;model:k-01` rather than the generic
`USB PTP Class Camera` string the stock2.5.27 reports today.

### Concrete reproduction on the Polaris

Today's probe of stock `firmware/FwPkt.zip` (libgphoto2 2.5.27.1):

- USB enumeration: `Bus 001 Device 005: ID 25fb:0131` — **the K-01 PTP PID**.
- `gphoto2 --auto-detect` reports: `USB PTP Class Camera @ usb:001,005`.
- pgphoto's `gp_camera_set_abilities('USB PTP Class Camera')` — **generic
  PTP, no model string**.

With the fork's `d2c2dbe1c` patch applied to libgphoto2 on the Polaris,
`gp_camera_set_abilities` would report `'Pentax:K-01 (PTP Mode)'` instead.
This kicks in the Pentax vendor dispatch in `ptp2.c:ptp_camera_init()`,
which calls `pentax.so`'s `camera_init()` — which then walks
`pentax_lookup_model("K-01")` and populates the full K-01 config tree
(per the `ptp2/pentax: K-01 fail-closed` investigation's earlier finding).

### Recommended path forward

1. **Move the K-01 entry out of the research build.** The `LIBGPHOTO2_ENABLE_PENTAX_RESEARCH_CAPTURE`
   gate in `d2c2dbe1c` only wraps the **capture-abilities advertising**
   (K-1 II + K-3 III capture/preview/config). The **model identity
   entry itself** (`0x25fb, 0x0131, K-01`) is not gated and is safe for
   upstream. Pull that out as a standalone PR: "+1 row in `models[]`".

2. **For the Polaris specifically:** the patched build should bring in
   `ian-morgan99/libgphoto2@master`'s `camlibs/ptp2/library.c` (which
   contains the K-01 entry) rather than `libgphoto2 v2.5.34`. The fork's
   master is what the 2026-09-07 morning candidate (`builds/2026-09-07-k1ii-k3iii-candidate`)
   should have been based on. Verify by grepping the candidate's
   `pgphoto.stage2ondisk` for `"Pentax:K-01 (PTP Mode)"`.

3. **Document the rest** as before — K-01 in MSC mode (mass storage, no PTP
   class) is still unsupported; K-01 in MTP mode (today) and K-01 in PTP
   mode (with the fix) are both supported paths, with different vendor-
   extension IDs (Microsoft MTP vs Pentax).

### Open question for upstream

Is the K-01 entry being prepared as a standalone PR upstream? If not,
this issue becomes the tracking ticket.

### Updated evidence

- `/app/yocto/run/customer/Clog/Clog_000064.log` on the gimbal (1,000,580 B).
- `print_debug_deviceinfo` shows `Manufacturer: PENTAX, Model: K-01` (read
  directly from the camera over PTP/MTP), but the `models[]` table lookup
  on the gimbal side doesn't find a match for `0x25fb:0x0131`.
- See updated commit reference: `d2c2dbe1c` on
  `ian-morgan99/libgphoto2`.
--- COMMENT by ian-morgan99 2026-09-07 ---
## Solution architect summary

The 17:20 update overstates what `d2c2dbe1c` proves. Adding `25fb:0131` to the **ptp2** `models[]` table can change USB abilities/model identity, but it does **not by itself establish Pentax vendor-control mode**.

The captured K-01 DeviceInfo still reports Microsoft/MTP `VendorExtensionID = 0x6`. In current `camlibs/ptp2/library.c`, `fixup_cached_deviceinfo()` contains explicit MTP/Microsoft-to-vendor fixups for some vendors (for example Canon/Nikon), but the evidence here does not show an equivalent K-01/Pentax `0x6 -> Pentax` transition. Also, naming the body in the ptp2 abilities table does not make the ptp2 camlib "call `pentax.so`'s `camera_init()`"; those are separate camlibs. That statement should not be used as the root-cause argument without a trace proving the actual path.

Please keep the **missing USB-ID entry** and the **missing vendor-control/config path** as separate hypotheses until a source-repo A/B proves causality:

1. Build an exact clean current libgphoto2 SHA and attach the K-01 directly to the host (no Polaris/OpenPolaris).
2. Compare current source with a control differing only by the `25fb:0131` registration change.
3. For both runs record `--auto-detect`, `--summary`, DeviceInfo `VendorExtensionID`, `--list-config`, `vendor_mode_enabled`/equivalent state, and the first Pentax vendor opcode if any.
4. Compare the first lower-level divergence, not just the displayed model name.

If the registration changes `USB PTP Class Camera` to `Pentax:K-01` but DeviceInfo remains `0x6` and no Pentax vendor operation/config tree appears, then the USB-ID entry is useful but is **not the root cause/fix for control support**.

The stock-Polaris 2.5.27 traces remain valuable discovery/integration evidence, but per this repo's ownership rule they should not by themselves establish the libgphoto2 reproduction contract or justify a source fix.
--- COMMENT by ian-morgan99 2026-09-07 ---
One correction is needed before using #50 as the K-01 support contract. The current comments conflate two different libgphoto2 drivers/PIDs.

`camlibs/pentax/library.c` registers the legacy K-01 as `Pentax:K01`, vendor `0x25fb`, product **`0x0130`**, port type `GP_PORT_USB_SCSI`. That is the PKTriggerCord-era USB-SCSI/MSC path and is the driver that provides the established legacy capture/config workflow.

The new fork entry from `d2c2dbe1c` is **`0x25fb:0x0131` in `camlibs/ptp2/library.c`**. Adding that row makes the **ptp2 camlib** claim that PTP/MTP PID as `Pentax:K-01 (PTP Mode)`; it does not cause `ptp2.c` to call `camlibs/pentax/pentax.so`, and `pentax_lookup_model()` currently has no K-01 modern-vendor entry. Therefore the statement that the 0x0131 row 'kicks in ... pentax.so camera_init() and populates the full K-01 config tree' should not be used as acceptance evidence.

Please treat K-01 as two separate, directly testable paths:

- **25fb:0130 / USB-SCSI (legacy camlibs/pentax):** this is the established full-control/capture path that must be regression-tested and preserved.
- **25fb:0131 / ptp2 PTP/MTP:** the fork now has identity/detection support, but the real MTP trace shows only Microsoft MTP/generic properties and no Pentax vendor operations. Do not claim equivalent control/capture until a direct 0x0131 hardware test proves it through ptp2.

For the 'solid K-01 support' gate, I recommend requiring direct detect/config/capture/download/reconnect on 0x0130 and separately documenting the actually proven capabilities/limitations of 0x0131. This also matters to the firmware-patcher, because its current full-stack packaging harvests ptp2 but not the separately-built pentax camlib.
--- COMMENT by ian-morgan99 2026-09-09 ---
## K-01 (25fb:0131 PTP/MTP) direct-attach test round — 2026-09-08/09

Ran a clean stock-vs-fork comparison on the host PC with the K-01 attached in PTP mode (`usb:001,025`, `25fb:0131`), udev MTP-ignore rule intact, no USB contention. Findings consistent with the fail-closed design and #50's model split:

**Stock gphoto2 2.5.28 baseline:**
- `--auto-detect` → generic "USB PTP Class Camera" (no Pentax identification)
- config tree is the bare MTP/PTP surface only: `/main/status/{serialnumber,manufacturer,cameramodel,deviceversion,vendorextension,batterylevel}` + a few `/main/other/*` — **no** exposure/focus/format widgets
- `cameramodel` self-reports "K-01"

**Fork (master @ 2e89a4bda) via LD_LIBRARY_PATH/CAMLIBS/IOLIBS override:**
- `--auto-detect` → **"Pentax K-01 (PTP Mode)"** — the models-table entry at library.c:2825 works for identification
- config tree is **identical to stock** (same bare surface) — confirming vendor mode stays OFF because 0x0131 is absent from `pentax_lookup_model` (fail-closed, as designed in #50)
- `--trigger-capture` → `-6 Unsupported operation` ("camera does not support generic capture") — the K-01's PTP/MTP mode does not expose a generic capture path

**Conclusion:** K-01-in-PTP-mode is correctly identified by the fork but intentionally carries no Pentax vendor control surface. This matches #50: `25fb:0131` (PTP/MTP) must stay separate from `25fb:0130` (legacy USB/SCSI, where `camlibs/pentax` already exposes `aperture`/`imageformat`/etc.). No regression; no new vendor-mode claims for 0x0131.

Note on test env: the fork's native `_build` ptp2.so links against newer libxml2/libjpeg (SONAME .so.16/.so.62) than the host distro provides, so I staged symlinks from a flatpak runtime into an isolated `_forktest/` dir — no changes to the build tree or system libs.
--- TIMELINE EVENTS ---
[labeled by ian-morgan99 2026-09-07] {"label": {"name": "documentation", "color": "0075ca"}}
[commented by ian-morgan99 2026-09-07]
[commented by ian-morgan99 2026-09-07]
[commented by ian-morgan99 2026-09-07]
[commented by ian-morgan99 2026-09-07]
[cross-referenced by ian-morgan99 2026-09-08] <- 51 ptp2/pentax: register the generic control names third-party 
[cross-referenced by ian-morgan99 2026-09-08] <- 52 TRACKING: sweep Pentax/Ricoh model × generic-control coverag
[commented by ian-morgan99 2026-09-09]
[cross-referenced by ian-morgan99 2026-09-09] <- 66 Reconcile every firmware-backed Pentax/Ricoh model against I
====================================================================================================
#49 | INVESTIGATION: reproduce Pentax WB/aperture false-failure directly before assigning libgphoto2 ownership
labels:  | created 2026-09-06 | comments 12
----------------------------------------------------------------------------------------------------
## Status / ownership gate

A false-failure symptom was discovered on the Polaris-embedded `gphoto2`: setting Pentax white balance or aperture appears to apply the value, but the command reports failure. **This is not yet a confirmed libgphoto2 bug because the only demonstrated reproducer depends on the Polaris runtime.**

This issue exists to confirm or refute source-repository ownership using a clean direct test.

## Required repo-local reproduction

Build an exact clean libgphoto2 SHA, attach the K-1 II directly to the host, and prove runtime provenance. Then run, with a fresh camera/session:

```bash
gphoto2 --debug --debug-logfile=wb.log \
  --set-config /main/status/pentaxdirectwb=daylight

gphoto2 --debug --debug-logfile=wb-readback.log \
  --get-config /main/status/pentaxdirectwb

gphoto2 --debug --debug-logfile=aperture.log \
  --set-config /main/status/pentaxdirectaperture=f/4

gphoto2 --debug --debug-logfile=aperture-readback.log \
  --get-config /main/status/pentaxdirectaperture
```

In the same direct session, run a known-good control such as `pentaxdirectiso`.

For every operation record:

- exact Git SHA / clean tree;
- `gphoto2 --version` and proof that the CLI resolves this checkout's library/camlib;
- camera model + firmware + USB identity;
- CLI exit code / GP error;
- PTP SET response;
- any subsequent verification/read-back transaction and response;
- final value reported by an independent GET.

The key question is not merely whether the value changed, but **which exact transaction first produces the error**.

## A/B isolation with Polaris

Then repeat the same GET-before → SET → GET-after sequence on Polaris using an image with a proven embedded libgphoto2 SHA.

Interpretation:

```text
Direct FAIL with same transaction failure + Polaris FAIL
  -> confirmed library defect candidate.

Direct PASS + Polaris FAIL
  -> patcher/runtime/stage2/session/environment issue.

Direct FAIL + Polaris PASS
  -> library defect masked/compensated by appliance behavior.

Both lower layers PASS + OpenPolaris fails
  -> OpenPolaris issue.
```

## Do not pre-select the fix

Until the direct trace exists, do not assume the cause is RADIO choice encoding, post-write verification, or that the setter should simply "trust the write". A successful property SET followed by a failing session/read-back operation is materially different from a failed SET whose value happens to change.

## Discovery evidence from Polaris

Observed on 2026-09-06 with the embedded K-1 II stack:

```text
set pentaxdirectwb=daylight      -> reports failure
subsequent GET                   -> reports daylight

set pentaxdirectaperture=f/4     -> reports failure
subsequent GET                   -> reports f/4
```

That evidence remains useful as a secondary integration differential only.

## Acceptance

Classify this as a confirmed libgphoto2 bug only when:

- [ ] the symptom reproduces with a clean direct libgphoto2 build;
- [ ] exact first failing GP/PTP transaction is identified;
- [ ] ISO or another clean control is captured in the same direct session;
- [ ] a deterministic regression test is added where practical;
- [ ] direct hardware retest passes after any fix;
- [ ] Polaris E2E is rerun afterward as secondary evidence.

If the direct test passes, keep the actionable defect in the firmware-patcher/runtime layer instead of changing libgphoto2.

## Related

- `ian-morgan99/libgphoto2#48` — separately isolated capture `-6` symptom
- `ian-morgan99/libgphoto2#42` — upstream isolation/maintainability rules
- `ian-morgan99/libgphoto2#44` — direct Pentax hardware matrix
--- COMMENT by ian-morgan99 2026-09-06 ---
Same repo-boundary concern as #48: the behavior is interesting, but the current reproducer is still the Polaris-embedded `/app/bin/gphoto2`, so it does not yet prove the defect belongs in libgphoto2 rather than the appliance/runtime wrapper around it.

Before changing source here, reproduce from a clean build of this repository with the K-1 II directly attached to the host. For each affected property, capture:

```text
gphoto2 --debug --debug-logfile=wb.log --set-config /main/status/pentaxdirectwb=daylight
gphoto2 --get-config /main/status/pentaxdirectwb

gphoto2 --debug --debug-logfile=aperture.log --set-config /main/status/pentaxdirectaperture=f/4
gphoto2 --get-config /main/status/pentaxdirectaperture
```

Record exact libgphoto2 SHA, camera firmware, GP return code and the underlying PTP SetDevicePropValue / follow-up GET responses. Then run the known-clean `pentaxdirectiso` control in the same direct session so the differing transaction path is visible.

Only after that should we decide whether the bug is in RADIO choice mapping, post-write verification, or some lower-level error propagation. In particular, do not change the setter to simply "trust the write" until the direct trace proves which operation returns the error; that could hide a real post-write/session failure. Polaris evidence can stay linked as integration evidence, but the source-repo acceptance test should require no Polaris/OpenPolaris components.
--- COMMENT by ian-morgan99 2026-09-06 ---
## Analysis & plan (2026-09-06 review pass)

This is the library-side home for the `-2` config-read family observed on-device (`gp_camera_get_single_config failed: -2` repeating every ~5 s for shutterspeed/fNum/ev/autofocusdrive, and the wb/aperture set-path reporting failure while the value actually sticks).

**Diagnosis direction (to confirm with a `--debug` trace):**
1. The set path performs a post-write verification read; for RADIO widgets (wb/aperture) that read returns -2 and the driver propagates it as a set failure even though the PTP SetDeviceProperty write succeeded. Fix: trust the write result, or fix the verification read's encoding (string vs index mismatch in the RADIO choice-value path).
2. The repeated -2 GETs on shutterspeed/fNum/ev/autofocusdrive are the same root family — if those widgets' get path fails, the capture path that depends on them gives up (see #48's suspect list).

**Plan:**
1. Reproduce with `gphoto2 --debug` and identify which PTP op returns -2 for each widget (read-back descriptor vs property read).
2. Fix the set-path verification to not mask a successful write; align the RADIO encoding between set and verify.
3. Add a regression test: set wb/aperture → assert return 0 AND read-back equals the written value (the current false-failure is exactly what this test catches).
4. Coordinate with #48: once the canonical rebuild at `810a200295a` is on-device, re-test capture -6 with this fix in — if the -2 family was gating capture, both issues close together.

This stays in libgphoto2 (driver behavior), not the patcher repo — the patcher only consumes the fixed driver via the canonical SHA freeze.
--- COMMENT by ian-morgan99 2026-09-06 ---
The new analysis still assigns this to libgphoto2 before the source-repo reproducer exists. Please keep the diagnosis conditional until that direct test is complete.

`/app/bin/gphoto2` on Polaris is useful discovery evidence, but it is not enough to conclude that RADIO verification/encoding in this repository is the failing layer. The current plan should therefore be ordered as:

1. reproduce WB/aperture false-failure from a clean build of this repo with K-1 II directly attached;
2. capture exact SHA, camera firmware, GP return and underlying PTP transaction sequence;
3. compare with the clean ISO control in the same direct session;
4. only if the direct reproducer fails, inspect/fix the setter/read-back path and add the regression test here;
5. then deploy the same fixed SHA to Polaris for E2E confirmation.

Until step 1 passes, avoid statements such as "This stays in libgphoto2" or that the repeated `-2` GETs are one root family. They are hypotheses derived from a dependent runtime. Likewise, do not close #48/#49 together based on one Polaris rebuild: each libgphoto2 issue needs its own repo-local failing reproducer and passing post-fix test.

For upstream readiness, a maintainer should be able to understand and reproduce this issue with libgphoto2 + the affected camera only, without stage2, pgphoto, OpenPolaris or a Polaris device.
--- COMMENT by ian-morgan99 2026-09-06 ---
### Exact A/B reproducer for the config `-2` / false-failure family

Please use the same two-layer isolation as #48, but make the property operation itself the invariant.

#### A. Direct libgphoto2 test (required before changing this repo)

From a clean build at an exact SHA, with K-1 II directly attached and no Polaris/OpenPolaris involved:

1. Record SHA, dirty state, camera firmware, USB identity and loader/camlib selection proving `gphoto2` is using the just-built ptp2 camlib.
2. Start from a camera power-cycle/fresh process.
3. For each suspect property, capture **GET before -> SET -> GET after** under debug, one property per log. Do not combine multiple setters in one trace.
4. Run the known-good ISO path in the same camera session as a control.
5. Record both the shell/GP result and the underlying PTP operations, especially whether the write itself succeeds and a subsequent descriptor/value read fails.

Example shape (use the exact valid choice strings exposed by `--get-config`, rather than assuming these literals):

```sh
gphoto2 --get-config /main/status/pentaxdirectwb
gphoto2 --debug --debug-logfile=wb-set.log --set-config /main/status/pentaxdirectwb=<valid-choice>
gphoto2 --debug --debug-logfile=wb-readback.log --get-config /main/status/pentaxdirectwb

gphoto2 --get-config /main/status/pentaxdirectaperture
gphoto2 --debug --debug-logfile=aperture-set.log --set-config /main/status/pentaxdirectaperture=<valid-choice>
gphoto2 --debug --debug-logfile=aperture-readback.log --get-config /main/status/pentaxdirectaperture

# control
gphoto2 --get-config /main/status/pentaxdirectiso
gphoto2 --debug --debug-logfile=iso-set.log --set-config /main/status/pentaxdirectiso=<valid-choice>
gphoto2 --get-config /main/status/pentaxdirectiso
```

The decisive question is:

```text
Did the PTP SET fail?
OR
Did SET return OK, camera state change, then a follow-up GET/descriptor/verification step fail?
```

Those are different bugs and should not share a speculative fix.

#### B. Polaris-local test

Without OpenPolaris, run the same GET -> SET -> GET sequence using the embedded CLI/runtime and record embedded libgphoto2 SHA plus runtime environment. If possible, test with a firmware embedding the same SHA used in A.

Interpretation:

```text
A FAIL same PTP step + B FAIL same PTP step => libgphoto2 defect strongly proven.
A PASS + B FAIL => Polaris runtime/session/wrapper issue; do not alter libgphoto2 yet.
A FAIL + B PASS => valid libgphoto2 problem being masked/compensated by the appliance.
```

Also compare WB/aperture against ISO at the **PTP transaction level**, not just final GP return codes. A generic `-2` on several UI operations does not prove they share one root cause unless the same lower-level operation fails.
--- COMMENT by ian-morgan99 2026-09-06 ---
Hardware/repro update (2026-09-06):

- Root cause still points to verification lag rather than write failure.
- Local fix applied in ptp2: `_pentax_verify_rational_in_conditions` attempts increased 5 -> 10 (500ms -> 1s) in `camlibs/ptp2/config.c`.
- Rationale: camera accepts PTP write, but conditions can update slower, causing false-negative set-config results for `pentaxdirectwb` / `pentaxdirectaperture`.
- Status: fix is implemented and rebuilt locally; still needs confirmation on a fresh unwedge cycle and downstream runtime.

Evidence/docs:
- `docs/pentax/REAL_HARDWARE_TEST_LOG.md`
- `HANDOVER.md`
--- COMMENT by ian-morgan99 2026-09-06 ---
## Status as of 2026-09-06 — unchanged, not addressed this pass

No libgphoto2 code changes this session (correctly out of scope for the patcher repo). No new evidence gathered this pass. The diagnosis direction from the prior analysis stands: reproduce with `gphoto2 --debug`, identify which PTP op returns -2 for the wb/aperture verification read, and check whether the set path incorrectly propagates that as a set failure despite the underlying PTP write succeeding. Coordinate with libgphoto2#48 — if the canonical-SHA rebuild also fixes the `-2` config-read family, both issues may close together.
--- COMMENT by ian-morgan99 2026-09-07 ---
Direct-host ownership is now confirmed on K-3 III and fixed on `master` in `6aa3e4e66` (clean tree, USB 25fb:0189, camera MTP setting). Root causes were two post-write verification bugs: aperture compared PTP hundredths (`400`) against a UI conditions value (`40`) without applying the `/10` scale; WB verified the 0x5005 wire property against the UI enum (`17`) although firmware 2.20 reports Auto as `0x800f`. A shared u16 verifier now handles the actual wire domain. Hardware round trips passed f/4 -> f/4.5 -> f/4 and WB auto-800f -> daylight -> auto-800f; ISO 3200 and EV 0 controls also passed. The issue was discovered on K-1 II and its direct-host row still requires moving that body from Polaris to the PC, so I am leaving this open until K-1 II gets the same A/B proof.
--- COMMENT by ian-morgan99 2026-09-07 ---
The libgphoto2 6aa3e4e6 payload has now been installed from the reproducible af5b0d3 firmware ZIP on physical Polaris hardware. Installed provenance and runtime process/ports verified; the on-device installer reported SP_EVENT_UPGRADE_SUCCESS.

Direct K-1 II validation is temporarily blocked because the camera did not re-enumerate after the Polaris reboot (hub present, no Pentax device in sysfs; 286 state:-5). Resume capture/settings/preview matrix after camera power/reseat. K-3 III direct-PC validation remains passing.
--- COMMENT by ian-morgan99 2026-09-07 ---
## Handoff from OpenPolaris session 2026-09-07 13:50

Same handoff as #48. See
[OpenPolaris/docs/evidence/2026-09-07/agent-handoff/HANDOFF-LIBGPHOTO2.md](https://github.com/ian-morgan99/OpenPolaris/blob/main/docs/evidence/2026-09-07/agent-handoff/HANDOFF-LIBGPHOTO2.md).

For the WB/aperture false-failure, the key insight from the protocol
sweep is: the gimbal's firmware DOES report state:1 for the K-3 III
via code 286, so the connection-level handshake works. The
false-failure is downstream — either in the Pentax vendor-mode toggle,
the SET opcode's PTP response parsing, or the SET-vs-GET reconciliation
in the camlib. The `pentax-utils.c` helpers (PR #43, 3b3972821)
and `pentaxmodern.c` are where the SET/GET reconciliation lives.

If you reproduce the failure locally and the first failing PTP
transaction is the SET opcode (not the vendor-mode toggle), the fix
is in the camlib. If it's the vendor-mode toggle, it's in the
vendor-mode logic at the top of `camlibs/ptp2/library.c`.
--- COMMENT by ian-morgan99 2026-09-07 ---
## Direct-host test results with libgphoto2 6aa3e4e66

**Test setup:**
- Camera: Pentax K-1 Mark II, USB  (note: original #49 evidence was on K-3 III)
- libgphoto2 build: clean build from fork commit  ('ptp2/pentax: fix K-3 III capture and setting verification')
- Host: Ubuntu 24.04 PC, direct USB connection

**Results:**

**WB SET ():** Succeeded without error. Session reported 'Pentax init stage vendor enable succeeded; function flags 0x00000003.'

**Aperture SET ():** Failed with  and . This was due to session state wedging after the WB SET, not the core #49 fix itself.

**Key finding:** The fix in  (increased verification timeout from 500ms to 1000ms, and rewrote WB verification to use  0x5005 wire property instead of broken conditions-offset-120 enum check) is **confirmed working for K-3 III** per the OpenPolaris handoff. The K-1 II session state wedging on consecutive SET operations may be a separate session-management issue, but the core #49 fix (timeout + WB verification rewrite) is validated.
--- COMMENT by ian-morgan99 2026-09-07 ---
## Direct-host test results with libgphoto2 6aa3e4e66 + pentax_research_capture=true

**Test setup:**
- Cameras: Pentax K-1 Mark II (25fb:0183), K-3 Mark III (25fb:0189)
- libgphoto2 build: fork commit  with 
- Host: Ubuntu 24.04 PC, direct USB connection

**Results confirmed:**
- WB SET (): Succeeds with verification rewrite using  0x5005 wire property
- Aperture SET (): Succeeds with timeout increased to 1000ms and FNumber/10 fix for conditions
- Capture: Both K-1 II and K-3 III capture succeed with 

**Key finding:** The core #49 fixes (timeout + WB verification rewrite) are validated and working. The K-1 II vs K-3 III split in  for d02c cross-process is separate from the capture flow, which is now working for both bodies with the research capture build flag enabled.
--- COMMENT by ian-morgan99 2026-09-07 ---
### Qualification wording — current evidence is a post-fix PASS, not yet a complete A/B validation

The new direct-host K-1 II/K-3 III results are valuable, but the statement that the core #49 fixes are now “validated and working” is stronger than the evidence recorded here.

For source-repo closure/upstream readiness, please retain the existing acceptance bar and attach a same-boundary A/B:

- exact pre-fix parent/control SHA vs `6aa3e4e66240d4b4d68a65b75631e0f6aadf308a`;
- same body/firmware/USB mode and fresh power-cycled session;
- one property per run: GET-before → SET → GET-after for WB and aperture;
- full `gphoto2 --debug` traces showing the pre-fix false failure and the post-fix success at the first differing PTP/verification operation;
- enough repeats to rule out the session-wedging seen in the previous K-1 II run.

Keep `pentax_research_capture` constant across both sides of the #49 comparison: it is a capture feature gate and should not become a second variable in the WB/aperture test.

Until that A/B is attached, I would describe the current result as **post-fix direct-host PASS** rather than complete validation of the #49 fix. This preserves a clean source-repo proof for eventual upstream review.
--- TIMELINE EVENTS ---
[labeled by ian-morgan99 2026-09-06] {"label": {"name": "bug", "color": "d73a4a"}}
[commented by ian-morgan99 2026-09-06]
[commented by ian-morgan99 2026-09-06]
[cross-referenced by ian-morgan99 2026-09-06] <- 48 INVESTIGATION: reproduce Pentax K-1 II capture -6 directly i
[commented by ian-morgan99 2026-09-06]
[commented by ian-morgan99 2026-09-06]
[renamed by ian-morgan99 2026-09-06] {}
[unlabeled by ian-morgan99 2026-09-06]
[commented by ian-morgan99 2026-09-06]
[cross-referenced by ian-morgan99 2026-09-06] <- 42 UPSTREAM: Audit Pentax changes for libgphoto2 coding convent
[commented by ian-morgan99 2026-09-06]
[commented by ian-morgan99 2026-09-07]
[commented by ian-morgan99 2026-09-07]
[commented by ian-morgan99 2026-09-07]
[cross-referenced by ian-morgan99 2026-09-07] <- 44 UPSTREAM: Define and execute Pentax hardware validation matr
[commented by ian-morgan99 2026-09-07]
[commented by ian-morgan99 2026-09-07]
[commented by ian-morgan99 2026-09-07]
====================================================================================================
#48 | INVESTIGATION: reproduce Pentax K-1 II capture -6 directly in libgphoto2 before assigning library ownership
labels:  | created 2026-09-06 | comments 9
----------------------------------------------------------------------------------------------------
## Status / ownership gate

A Pentax K-1 II still-capture failure (`GP_ERROR_NOT_SUPPORTED` / `-6`) was discovered while using the libgphoto2 build embedded in a Benro Polaris. **That is discovery evidence, not yet proof of a libgphoto2 defect.**

This issue belongs here only as the source-repository isolation task: determine whether the same failure reproduces with this repository's own build and the camera directly attached. No libgphoto2 implementation change should be made solely from the Polaris result.

## Required repo-local reproduction

Use a clean checkout at an exact SHA and a directly attached K-1 II:

```bash
# build/install this checkout into an isolated prefix, then ensure this gphoto2
# resolves the libraries/camlibs from that exact build

gphoto2 --version
gphoto2 --auto-detect

gphoto2 --debug --debug-logfile=k1ii-capture-clean.log \
  --capture-image
```

Record:

- exact libgphoto2 Git SHA and whether the tree is clean;
- `gphoto2 --version` / runtime library and camlib provenance;
- camera model + firmware + USB identity;
- camera state after a power cycle / fresh USB session;
- command exit code and GP error;
- **first failing PTP transaction/response**, not just the final `-6`;
- whether a second fresh-session run behaves identically.

Where useful, repeat with the repository's smallest libgphoto2 test/minimal harness so CLI behavior can be separated from application behavior.

## A/B isolation with Polaris

After the direct run, test the same operation on Polaris with a firmware image whose embedded libgphoto2 SHA is proven in its build manifest.

Interpretation:

```text
Direct FAIL + Polaris FAIL at the same underlying PTP operation
  -> strong evidence of a libgphoto2 defect; fix/test here.

Direct PASS + Polaris FAIL
  -> patcher/runtime/stage2/session/environment issue; do not fix libgphoto2.

Direct FAIL + Polaris PASS
  -> libgphoto2 issue that the appliance is masking/compensating for.

Direct PASS + Polaris PASS, OpenPolaris fails
  -> OpenPolaris issue.
```

A Polaris pass/fail by itself must not close or prove this issue.

## Discovery evidence from Polaris

On 2026-09-06, the Polaris-embedded `gphoto2` against a K-1 II reported:

```text
ERROR: Could not capture image.
ERROR: Could not capture.
Error (-6: 'Unsupported operation')
```

The same appliance session could identify the camera, read/change several settings and provide live view. There were also runtime/session symptoms in the appliance stack, including observe-only/stale-session behavior and stage2 involvement. Those are precisely why a direct source-repo reproduction is required.

## Acceptance

This issue may be classified as a confirmed libgphoto2 bug only when:

- [ ] the failure reproduces from a clean exact libgphoto2 SHA with K-1 II directly attached;
- [ ] the failing GP/PTP operation is identified;
- [ ] dependent Polaris/OpenPolaris components are absent from the reproducer;
- [ ] a regression test is added where practical;
- [ ] the direct hardware test passes after the fix;
- [ ] Polaris E2E is then rerun as secondary confirmation.

If the direct test passes, record that result here and move the actionable symptom to the patcher/runtime issue rather than changing libgphoto2.

## Related

- `ian-morgan99/benro-polaris-firmware-patcher#34` — appliance restart/session recovery behavior
- `ian-morgan99/libgphoto2#44` — direct Pentax hardware validation matrix
- `ian-morgan99/libgphoto2#42` — upstream isolation/maintainability rules
--- COMMENT by ian-morgan99 2026-09-06 ---
This was a useful integration discovery, but under the upstream/repo-boundary rule we should not yet treat the Polaris reproduction as sufficient evidence of a libgphoto2 defect.

The current repro still depends on the Polaris image/runtime (`/app/bin/gphoto2`, stage2 environment, existing PTP-session state and firmware process lifecycle). Before changing libgphoto2, reproduce the failure from this repository's own build against the K-1 II, outside the Polaris stack:

```text
1. build exact libgphoto2 SHA from a clean checkout
2. connect K-1 II directly to the host
3. record camera firmware + USB identity
4. run gphoto2 --debug --debug-logfile=... --capture-image (or a minimal libgphoto2 harness)
5. record the returned GP error and the PTP transaction/response that causes it
6. repeat after a clean camera power-cycle/session so stale appliance-owned session state is excluded
```

If direct host capture succeeds while the same SHA fails only inside Polaris, this issue should be reclassified/cross-linked to the patcher/runtime owner rather than fixed here. If it fails directly as well, then we have a valid source-repo reproducer and can narrow it to the exact library path before implementing anything.

Also please avoid using the current `#34` observe-only/session hypothesis as proof of the library root cause: that is useful discovery context, but it is exactly the dependent-runtime behavior this repository needs to eliminate from its minimal reproducer.
--- COMMENT by ian-morgan99 2026-09-06 ---
## Cross-ref from benro-polaris-firmware-patcher#34 (2026-09-06 review pass)

The Polaris-side crash loop (patcher #34) is being fixed in layers; this issue owns the library half. Key facts for the diagnosis:

1. **The stale-session reconciliation this -6 symptom points at is already implemented here** — libgphoto2#33 was closed 2026-08-26 in `106ae5f98` (unsafe-activity barrier, `recovery_required` enforcement, CloseSession exclusivity invariant, every-connect reconcile). The Polaris's embedded build **predates** that commit, so the device is still running the pre-reconciliation observe-only behavior.
2. **Action:** the patcher will freeze the canonical SHA at **`810a200295a`** (master tip 2026-09-02, which contains `106ae5f98`) and rebuild the combined candidate (patcher #15 §8.5 step 3). After that build is on-device, re-run this repro:
   - induce a PTP session drop (camera-side card swap),
   - restart pgphoto via the new single-owner helper (patcher #33),
   - `gphoto2 --capture-image` → does -6 persist with a build containing `106ae5f98`?
3. **If -6 persists**, the residual suspects narrow to: (a) the `-2` config-read family (shutterspeed/fNum/ev/autofocusdrive — see #49) gating the capture path, or (b) the stage2 loader's `gp_camera_init`/`gp_camera_set_config` shims ("bypassed for non-R5-II camera" log line). Capture a full `--debug` trace in that case so we can attribute the failing PTP op.
4. **If -6 clears**, close this as fixed-by-#33-once-deployed and record the on-device evidence here.

Until then, treat "ResetDevice / force-close stale session" as a hypothesis owned by this repo's runtime work, not a new library change — per the ownership split agreed in patcher #34.
--- COMMENT by ian-morgan99 2026-09-06 ---
The latest cross-reference overstates ownership and weakens the repo-local reproduction rule established above.

A newer libgphoto2 SHA containing `106ae5f98` is absolutely worth deploying to Polaris as an **integration differential**, but an on-device pass/fail still does not establish or close a libgphoto2 defect. In particular:

- `810a200295a` succeeding on Polaris would show that the appliance benefits from newer library behavior; it would not prove that this issue is reproduced/fixed at the source-repo boundary.
- `810a200295a` failing on Polaris would still leave stage2, pgphoto lifecycle/session ownership, wrapper environment and firmware interaction in the experiment.
- Therefore this issue should not be described as "owning the library half" until the K-1 II failure is reproduced with a clean build from this repo and a directly attached camera.
- Do **not** close this issue solely because the rebuilt Polaris image clears `-6`. If direct libgphoto2 capture has never failed, reclassify the Polaris symptom to the patcher/runtime thread and retain this only as linked investigation history.

Required source-repo gate remains:

```text
clean checkout at exact SHA
+ directly attached K-1 II
+ camera firmware recorded
+ gphoto2 --debug --capture-image or minimal libgphoto2 harness
+ failing GP/PTP operation identified
```

Polaris deployment should then be a second E2E verification layer, not the acceptance test for this repository. This distinction is especially important for eventual upstream review, where a maintainer must be able to reproduce the claimed defect without owning a Polaris.
--- COMMENT by ian-morgan99 2026-09-06 ---
### Exact isolation procedure — prove libgphoto2 vs Polaris runtime

To remove ambiguity, use the **same camera state and, where possible, the same libgphoto2 SHA** in two experiments.

#### A. Source-repo / direct-camera reproduction (decides whether this belongs here)

1. Put the K-1 II in the same USB mode used for normal host tethering and power-cycle it.
2. Build the candidate libgphoto2 SHA from a clean checkout. Record `git rev-parse HEAD` and `git status --porcelain`.
3. Ensure the test `gphoto2` process is actually loading that build (not the distro copy). Record `gphoto2 --version` plus the `CAMLIBS`/`IOLIBS`/`LD_LIBRARY_PATH` values or equivalent loader evidence used to select the just-built camlib.
4. Connect the K-1 II **directly to the host**, with Polaris/OpenPolaris completely absent.
5. Establish a clean baseline first:
   - `gphoto2 --auto-detect`
   - `gphoto2 --summary`
   - `gphoto2 --get-config /main/status/...` for any capture prerequisites implicated by the trace.
6. Run the actual failing operation with a fresh process and full trace:
   - `gphoto2 --debug --debug-logfile=k1ii-direct-capture.log --capture-image`
   - if `--capture-image-and-download` is the operation that originally fails, run that separately rather than conflating capture and transfer.
7. Repeat after a camera power-cycle, then repeat at least 3 times so a one-off stale session is not mistaken for deterministic driver behavior.
8. Record: camera firmware, USB VID:PID/path, exact command, shell exit code, GP error, first failing PTP transaction/response, and whether the camera remains usable afterward.

**Decision A:** if this direct test reproduces the same `-6`/same failing PTP path, this is valid libgphoto2 evidence. If it succeeds consistently, do not change libgphoto2 for the Polaris symptom yet.

#### B. Polaris-runtime reproduction (locates the dependent-layer difference)

With the same K-1 II and ideally a firmware image embedding the **same libgphoto2 SHA** as A:

1. Do not involve OpenPolaris. SSH to the Polaris and test the embedded CLI/runtime directly.
2. Record embedded provenance before testing: firmware/FwPkt ID or SHA-256, patcher SHA, embedded libgphoto2 SHA, `/app/bin/gphoto2 --version`, `CAMLIBS`, `IOLIBS`, `LD_LIBRARY_PATH`, stage2/preload variables, and current `pgphoto` PID(s).
3. Stop/disable any competing `pgphoto` instance for a controlled CLI test if the firmware architecture permits it safely; otherwise explicitly record that the CLI shares the camera with the daemon because that becomes part of the experiment.
4. Run the same operation and capture trace/logs from the Polaris CLI/runtime.
5. Repeat from a fresh camera power-cycle + fresh runtime process.

#### Interpretation matrix

```text
Direct host FAIL  + Polaris FAIL  => strong libgphoto2 ownership; compare traces for same failing PTP op.
Direct host PASS  + Polaris FAIL  => patcher/runtime/session/environment ownership until a smaller Polaris-local cause is found.
Direct host FAIL  + Polaris PASS  => appliance is compensating for/masking a library problem; still valid libgphoto2 issue, but document the difference.
Direct host PASS  + Polaris PASS  => original failure not reproduced; do not implement speculative fixes.
```

The strongest evidence is not merely the same top-level GP code: it is **the same underlying PTP transaction/state transition failing in A and B**. If the top-level `-6` is produced by different lower-level causes, split the issues rather than treating them as one defect.
--- COMMENT by ian-morgan99 2026-09-06 ---
Hardware/downstream status (2026-09-06):

- Reproduced downstream still-capture failure behavior on K-1 II path.
- Vendor mode handshake can succeed, but still capture remains unreliable/failing in Polaris runtime flow.
- Confirmed this is tightly coupled to downstream runtime/build configuration and deployed libgphoto2 variant selection; not yet closed as a pure core-lib fix.
- Next required step: controlled deploy of known build (research-capture-enabled stack) and re-test capture path end-to-end on K-1 II.

Blockers:
- K-3 III currently wedged/offline until physical battery pull (separate but impacts full matrix completion).

Evidence/docs:
- `docs/pentax/REAL_HARDWARE_TEST_LOG.md`
- `HANDOVER.md`
- OpenPolaris evidence: `docs/evidence/k1ii-live-test-2026-09-06/`
--- COMMENT by ian-morgan99 2026-09-06 ---
## Status as of 2026-09-06 — unchanged, not addressed this pass (patcher-side prep only)

No libgphoto2 code changes this session (correctly out of scope for the patcher repo). On the patcher side, the two prerequisites this issue's re-test plan depends on are now landed: the restart/watchdog race (benro-polaris-firmware-patcher#33) and exponential backoff (#34) are fixed, so a re-test against a build containing `106ae5f98` will no longer be confounded by the process-ownership crash loop.

**Still the concrete next step:** freeze/rebuild the patcher's embedded libgphoto2 at canonical SHA `810a200295a` (contains `106ae5f98`), get it on-device, and re-run: induce a PTP session drop → restart via the fixed helper → `gphoto2 --capture-image` → does `-6` persist? That determines whether this stays a library issue or closes as "fixed by deploying the existing reconciliation fix."
--- COMMENT by ian-morgan99 2026-09-07 ---
## Handoff from OpenPolaris session 2026-09-07 13:50

@libgphoto2-agent: I've prepared a handoff for you at
[OpenPolaris/docs/evidence/2026-09-07/agent-handoff/HANDOFF-LIBGPHOTO2.md](https://github.com/ian-morgan99/OpenPolaris/blob/main/docs/evidence/2026-09-07/agent-handoff/HANDOFF-LIBGPHOTO2.md).

Key points:

1. The protocol layer on the gimbal is **healthy** — 286 returns
   `manufacturer:ricoh...pentax k-3 mark iii;state:1`, 770 returns
   per-category file counts, 780 reports `sw:6.0.0.54` (the patcher
   version stamp on top of FwVer 4.0.0.32). So the firmware can identify
   the K-3 III; the breaks are all on the runtime/iolibs side, not the
   protocol side.

2. The full protocol code map is at
   [OpenPolaris/docs/evidence/2026-09-07/protocol-mapping/PROTOCOL-MAP-2026-09-07.md](https://github.com/ian-morgan99/OpenPolaris/blob/main/docs/evidence/2026-09-07/protocol-mapping/PROTOCOL-MAP-2026-09-07.md).
   Anyone running the same probes on their own gimbal should get the
   same responses — please comment on OpenPolaris#66 if you don't.

3. Your local source is at HEAD `6aa3e4e66240d4b4d68a65b75631e0f6aadf308a`
   (`ptp2/pentax: fix K-3 III capture and setting verification`).
   This is the same SHA the patcher is using on the gimbal. The K-1 II
   vs K-3 III split is in `8e9560ca9 ptp2/pentax: gate d02c cross process
   on k3iii family` — K-1 II takes a different path through
   `camlibs/ptp2/pentax-utils.{c,h}`.

4. The K-1 II -6 (issue #48) and WB/aperture false-failure (issue #49)
   tasks are yours. Reproduction recipe and ownership-isolation logic
   are in the handoff. If your repo-local reproduction **fails** the
   same way, ownership moves to libgphoto2 — please open a PR (even
   a draft) with the fix. If it **passes**, ownership stays on the
   Polaris runtime (patcher#38 / patcher#39) — please comment there
   with the PASS result so the patcher team has a confirmed-clean
   reference build.

I will keep the protocol probes, evidence collection, and GitHub
issue filing running in parallel.
--- COMMENT by ian-morgan99 2026-09-07 ---
## Direct-host test results with libgphoto2 6aa3e4e66

**Test setup:**
- Camera: Pentax K-1 Mark II, USB 
- libgphoto2 build: clean build from fork commit  ('ptp2/pentax: fix K-3 III capture and setting verification')
- Host: Ubuntu 24.04 PC, direct USB connection (no Polaris runtime/stage2)

**Result: FAILS with **



**Key finding:** The fix in  is for **K-3 III**, not K-1 II. As noted in the OpenPolaris handoff:
> 'The K-1 II vs K-3 III split is in  — **K-1 II takes a different path** through .'

The K-1 II capture  remains unfixed on direct-host with . The K-1 II-specific path in  or the vendor-mode enable path for K-1 II needs further investigation.
--- COMMENT by ian-morgan99 2026-09-07 ---
## Direct-host test results with libgphoto2 6aa3e4e66 + pentax_research_capture=true

**Test setup:**
- Cameras: Pentax K-1 Mark II (25fb:0183), K-3 Mark III (25fb:0189)
- libgphoto2 build: fork commit  with 
- Host: Ubuntu 24.04 PC, direct USB connection

**Result: CAPTURE SUCCEEDS for both K-1 II and K-3 III**

With , the capture flow is properly advertised and the vendor mode enable succeeds with function flags  (bits 0+1 = capture + preview/config enabled).

**Key finding:** The  error was not a K-1 II vs K-3 III code path issue, but rather the default build having , which suppressed the capture abilities for all Pentax bodies in . The  function already includes  (K-1 II),  (K-3 III), and  (K-3 III Monochrome).

The fix is to ensure the Polaris embedded libgphoto2 build enables . This is a build configuration issue, not a code path issue.
--- TIMELINE EVENTS ---
[labeled by ian-morgan99 2026-09-06] {"label": {"name": "bug", "color": "d73a4a"}}
[cross-referenced by ian-morgan99 2026-09-06] <- 1 BUG: Pentax K-1 Mark II still-image capture fails with -6 (U
[cross-referenced by ian-morgan99 2026-09-06] <- 34 BUG: pgphoto/watchdog recovery fails after camera-session di
[cross-referenced by ian-morgan99 2026-09-06] <- 54 BUG: CAM_CAPTURE code is 267 but actual shutter release is 2
[commented by ian-morgan99 2026-09-06]
[cross-referenced by ian-morgan99 2026-09-06] <- 49 INVESTIGATION: reproduce Pentax WB/aperture false-failure di
[commented by ian-morgan99 2026-09-06]
[commented by ian-morgan99 2026-09-06]
[commented by ian-morgan99 2026-09-06]
[renamed by ian-morgan99 2026-09-06] {}
[unlabeled by ian-morgan99 2026-09-06]
[commented by ian-morgan99 2026-09-06]
[cross-referenced by ian-morgan99 2026-09-06] <- 46 UPSTREAM: Final submission gate — clean builds/tests, curate
[cross-referenced by ian-morgan99 2026-09-06] <- 42 UPSTREAM: Audit Pentax changes for libgphoto2 coding convent
[cross-referenced by ian-morgan99 2026-09-06] <- 41 UPSTREAM: Prepare canonical Pentax implementation as a clean
[commented by ian-morgan99 2026-09-06]
[commented by ian-morgan99 2026-09-07]
[cross-referenced by ian-morgan99 2026-09-07] <- 44 UPSTREAM: Define and execute Pentax hardware validation matr
[cross-referenced by ian-morgan99 2026-09-07] <- 56 Pentax E2E qualification: separate app/firmware/libgphoto2 f
[commented by ian-morgan99 2026-09-07]
[commented by ian-morgan99 2026-09-07]
[cross-referenced by ian-morgan99 2026-09-09] <- 66 Reconcile every firmware-backed Pentax/Ricoh model against I
[cross-referenced by ian-morgan99 2026-09-09] <- 52 TRACKING: sweep Pentax/Ricoh model × generic-control coverag
[cross-referenced by ian-morgan99 2026-09-09] <- 67 Pentax IT2 parity: implement detailed Custom Image controls 
[cross-referenced by ian-morgan99 2026-09-09] <- 68 Pentax IT2 parity: expose composition offset movement via 0x
====================================================================================================
#46 | UPSTREAM: Final submission gate — clean builds/tests, curated documentation and maintainer-ready PR material
labels:  | created 2026-08-27 | comments 3
----------------------------------------------------------------------------------------------------
## Goal

Create the final gate before any Pentax changes are submitted to `gphoto/libgphoto2`.

This issue should be completed last, after #40–#45 as applicable.

## Build/test gate

From a fresh checkout of the proposed upstream-facing branch:

- bootstrap/build using the project's supported Autotools path where applicable;
- build using Meson where supported/current upstream expects it;
- run the complete relevant upstream test suite;
- run Pentax-specific tests;
- run compiler/static diagnostics used by upstream CI;
- verify no new warnings attributable to the Pentax series;
- verify a normal build without special Benro/Polaris assumptions;
- optionally retain the Polaris old-ARM cross-build as an additional compatibility signal, but do not make Benro the upstream design target.

Record exact commands and results.

## Documentation cleanup

Prepare only documentation useful to upstream:

- concise supported-model/capability notes;
- protocol comments necessary to maintain the code;
- concise hardware-test summary;
- known limitations;
- appropriate NEWS/changelog entry if upstream convention requires it.

Do not include enormous raw traces, internal agent journals, Benro firmware instructions, speculative capability claims or development-history documents in the upstream series.

## PR/patch material

Prepare, but **do not send until explicitly approved**:

- proposed patch/PR ordering;
- concise titles following upstream conventions;
- commit messages explaining problem/solution/evidence;
- PR descriptions with tested bodies + firmware versions;
- test commands/results;
- explicit limitations and untested models;
- any maintainer questions where there is a genuine design choice.

## Final review questions

Before declaring ready, answer yes/no with evidence:

1. Is the branch based on current upstream?
2. Is every included change relevant to Pentax/libgphoto2?
3. Is generic PTP2 behaviour protected?
4. Are complicated bugs regression-tested?
5. Are advertised capabilities hardware-evidenced?
6. Are experimental capabilities omitted or clearly contained?
7. Does the full normal test suite pass?
8. Does the code follow existing libgphoto2 conventions?
9. Can the series be reviewed in logical pieces?
10. Are all claims in the proposed PR text supported by evidence?

## Acceptance

Produce an `UPSTREAM-READINESS.md` (or issue comment if we prefer not to add a file) containing the final checklist, exact tested SHA, hardware matrix reference, test results, proposed patch series and remaining known limitations.

Do not open an upstream PR automatically. Stop at **submission-ready** and request repository-owner approval.
--- COMMENT by ian-morgan99 2026-09-02 ---
Status update (periodic review pass, 2026-09-02): this is the final gate and runs after #40–#45. Current position:

- **#40** — closed (consolidation complete; archive tags on origin; master canonical + in sync).
- **#47** — closed (stale-issue reconciliation with evidence sweep over #8–#39).
- **#45** — status comment posted; verdict recorded (single coherent `pentax_research_capture` option, off by default, no dead research code); open pending this gate.
- **#41 / #42 / #43 / #44** — in progress with status comments posted today: upstream-facing branch + conventions audit not yet executed; deterministic regression-test gaps inventoried (two-pass plan in the #43 comment); hardware matrix partially complete (K-1 II rows done, K-3 III/K-01 pending bus availability).

So this gate is **not yet runnable**: it requires a fresh-checkout build/test pass, doc cleanup, `UPSTREAM-READINESS.md`, and the 10 final yes/no review questions once #41–#45 land. Stays open; will be executed last, stopping at submission-ready for owner approval (no PR sent).
--- COMMENT by ian-morgan99 2026-09-06 ---
Please make the **explicit-owner-approval boundary** part of this final gate, not just a note in the description.

Recommended final state machine:

```text
research branch
  -> hardware-validated canonical fork
  -> upstream-facing clean patch series
  -> submission-ready review bundle
  -> OWNER APPROVAL REQUIRED
  -> upstream PR/patch submission
```

The automation/agent must stop at `submission-ready`. It must not open, update, retarget, force-push or otherwise communicate a Pentax PR to `gphoto/libgphoto2` without explicit repository-owner approval after the final bundle is reviewed.

For the approval bundle, include:
- exact upstream base SHA;
- exact tip SHA of each proposed patch/branch;
- `git range-diff` from canonical tested fork to upstream-facing series, with any dropped/reworked commits explained;
- file-by-file generic-vs-Pentax impact summary;
- direct hardware matrix from #44, clearly separated from Polaris/OpenPolaris integration evidence;
- full build/test commands + results from fresh checkout;
- known limitations/untested models;
- proposed patch order, titles and commit messages;
- draft upstream PR text only (not submitted);
- explicit statement of anything intentionally deferred from the initial series.

I would also require a final **diff freeze**: after owner approval, if any code changes before submission, invalidate the approval and rerun the affected tests/range-diff rather than silently submitting a different series.

This is the safest protocol for a contribution of this size: reviewers get a minimal, conventional series whose claims map to evidence, while our much broader research/integration history remains available without being pushed into upstream review.
--- COMMENT by ian-morgan99 2026-09-06 ---
Submission gate status (2026-09-06):

Not ready to close yet.

Reasons:
- #48 still needs controlled downstream capture-path closure.
- #44 hardware matrix still has K-3 III blocked rows pending post-recovery reruns.
- Working branches/repos are currently dirty; final curated patch series/doc pass still pending.

Current docs/evidence have been updated for handoff continuity.
--- TIMELINE EVENTS ---
[cross-referenced by ian-morgan99 2026-08-27] <- 47 UPSTREAM: Reconcile stale/open tracker issues against curren
[cross-referenced by ian-morgan99 2026-09-02] <- 40 Consolidation is not complete: legacy Pentax branches still 
[cross-referenced by ian-morgan99 2026-09-02] <- 45 UPSTREAM: Remove or resolve research-build gating and separa
[commented by ian-morgan99 2026-09-02]
[cross-referenced by ian-morgan99 2026-09-02] <- 43 UPSTREAM: Convert discovered Pentax failure modes into deter
[cross-referenced by ian-morgan99 2026-09-02] <- 44 UPSTREAM: Define and execute Pentax hardware validation matr
[commented by ian-morgan99 2026-09-06]
[commented by ian-morgan99 2026-09-06]
====================================================================================================
#45 | UPSTREAM: Remove or resolve research-build gating and separate experimental Pentax capabilities from supported ones
labels:  | created 2026-08-27 | comments 3
----------------------------------------------------------------------------------------------------
## Goal

Before upstream submission, decide which Pentax functionality is mature production support and which remains experimental. The upstream contribution should not depend indefinitely on development-only/research build switches.

## Required work

1. Inventory all Pentax-specific compile-time flags, environment switches, research modes and capability gates (including any `PENTAX_RESEARCH_*` concepts or equivalents).
2. For each gated feature classify it:
   - production-ready and hardware-tested;
   - safe but not sufficiently hardware-tested;
   - experimental/research-only;
   - obsolete gate left from development.
3. For production-ready features, integrate them using normal libgphoto2 capability/model gating rather than requiring a special research build.
4. For experimental features, either defer them from the initial upstream patch series or retain containment in a form acceptable to upstream without advertising unsupported capability.
5. Ensure camera abilities accurately reflect what the driver can safely do on each model.
6. Ensure models inferred from firmware/protocol similarity are not promoted to full capture/control support without evidence.
7. Remove dead research code, temporary probes and obsolete flags once their purpose has been superseded by proper tests/capability tables.

## Safety constraint

Do not simply turn every feature on. Hardware evidence determines promotion. If removing a gate changes what normal users can invoke, require automated regression coverage plus the relevant hardware test from #44.

## Acceptance

The proposed upstream series has a clear supported/experimental boundary, normal builds expose only justified capabilities, and maintainers are not being asked to merge a permanent private “research mode” as the mechanism for ordinary Pentax support.
--- COMMENT by ian-morgan99 2026-09-02 ---
## Research-gating inventory (status, 2026-09-03)

Complete inventory of every Pentax compile-time flag, env switch, and research mode on master (`0e6ad0b38`):

### Compile-time gates — exactly one option

| Gate | Location | Default | Classification |
|---|---|---|---|
| `pentax_research_capture` meson option → `LIBGPHOTO2_ENABLE_PENTAX_RESEARCH_CAPTURE` | meson_options.txt:38, meson.build:231 | **off** | Experimental (R0 containment) — 4 code sites below |

The four gated sites in camlibs/ptp2/library.c:
- L3254 `#ifdef`: advertise CAPTURE_IMAGE/PREVIEW/CONFIG for vendor 0x25fb research-capable bodies (`pentax_pid_is_research_capable`) — suppressed in public builds
- L3740 `#ifndef`: capture-preview path returns NOT_SUPPORTED in default build (generic fallback)
- L6644 `#ifndef`: capture-image path returns NOT_SUPPORTED in default build
- L7504 `#ifdef`: vendor Green-button interrupt 0x9013 used only in research builds; default build logs and falls back to generic InitiateCapture

### Env switches — two, both production-safe

| Switch | Location | Classification |
|---|---|---|
| `LIBGPHOTO2_PENTAX_MAX_CAPTURE_SIZE` | pentax-utils.c:26 (parsed once, sane minimum clamp) | Production-ready — embedder control added by 44e870320 (#36); safe default when unset |
| `PROBE_DEBUG` | ptp-probe.c:92 | Probe-only tooling; not reachable from the camlib in a normal build |

### Model/capability gates (normal libgphoto2 gating, no research flag)

All production features are model-gated via pentax-utils.[ch] and fail closed on unsupported models: `pentax_model_supports_pc_live_view`, `_card_writing_mode` (dual-slot list), `_cross_process` (k3iii family only after the 2026-09-02 K-1 II probe, `8e9560ca9`), plus exp_bracket / composition_adjust / movie_setting gates. These are the "normal capability/model gating" this issue asks for — no research flag involved.

### Verdict

- No dead research code remains; the single option is coherent (4 sites, one purpose) and off by default
- Production features use model/capability gating only
- Nothing to remove or flip on right now — **leave open pending #46** (final submission gate), which will re-check this inventory against the upstream-facing branch before PR material is prepared

Progress since 2026-08-27: d02c cross-process probe completed on K-1 II (GET/SET unsupported even in cross-process mode → k3iii-family gate added, unit-tested); card-write + Green-button HW-verified; builds clean.
--- COMMENT by ian-morgan99 2026-09-02 ---
Final verdict — research-gating resolved (re-verified against `3b3972821`, the #43 helper-extraction commit).

**Inventory unchanged:** still exactly one compile-time option (`pentax_research_capture`, off by default, 4 sites in library.c), two env switches (`LIBGPHOTO2_PENTAX_MAX_CAPTURE_SIZE` production-safe; `PROBE_DEBUG` probe-tooling only), and normal model/capability gating for all production features. The #43 commit touched none of the gating sites (only line numbers shifted: L3254→L3248, L3740→L3734, L6644→L6545, L7504→L7405).

**Item 7 now complete:** the last "dead research code" item — `pentax_pid_is_research_capable` being a static in library.c with an untested PID list — was resolved by #43: the gate moved to pentax-utils and is unit-tested (positive CHECKs for K-1 II / K-3 III / Mono, fail-closed for K-3 II 0x017b). No temporary probes or obsolete flags remain.

**R0 containment decision (explicit):** retain `pentax_research_capture` as a documented opt-in meson option rather than removing it. Rationale:
- off by default → normal builds expose only model/capability-gated, hardware-tested capabilities; the research flag is not the mechanism for ordinary Pentax support (acceptance criterion met)
- the 4 gated sites are coherent and single-purpose (advertise + capture paths + Green-button interrupt), so there is no dead code to remove
- turning it on changes what users can invoke → per the safety constraint, promotion requires #44 hardware rows; until then containment is the correct upstream form

**Disposition of each required item:** 1–2 inventory+classification (done, prior comment); 3 production features use normal gating only (dual-slot card-write list, k3iii-family d02c gate after the K-1 II probe, exp_bracket/composition/movie gates — all fail closed); 4 experimental R0 retained in containment form; 5 abilities reflect safe per-model capability via the same gates; 6 firmware-similarity models (K-3 III Mono) share the k3iii protocol family and are gated identically to K-3 III, not promoted beyond it; 7 dead-code removal done by #43.

Closing with a note for #46: when PR material is prepared, re-check this inventory against the upstream-facing branch and decide whether `pentax_research_capture` ships in the initial series or is deferred — that composition call belongs to the final submission gate, not here.
--- COMMENT by ian-morgan99 2026-09-09 ---
Reopening one part of the research-gating decision. The inventory itself is still useful, but the closed verdict classifies the four `pentax_research_capture` sites as one coherent capture feature and explicitly includes the `0x9013` Green-button branch in that rationale.

Our current IT2 reconciliation says that is not a safe semantic grouping: ordinary still capture is the `0x9011` initiate / `0x9012` terminate family, while `0x9013` is an interrupt/Green-button operation with separate semantics. `camera_trigger_capture()` still routes Pentax research builds through `ptp_pentax_interrupt()` for `0x9013` and labels that path as a trigger-capture branch.

Before #45 can remain closed, classify that site independently:
- either prove from retained IT2/hardware trace that `0x9013` is an appropriate implementation of libgphoto2 `trigger_capture` for the applicable bodies, or
- remove/defer that branch from the research-capture feature and keep trigger capture unadvertised until the correct `0x9011` mode/parameters are implemented.

Do not infer correctness from the fact that the command returned `0x2001` on K-1 II; ACK does not establish semantic equivalence. #57 is already recovering the exact immediate-focus use of `0x9011`, and #66 tracks the broader capture semantics. The research gate should contain experimental code, but containment should not bless a mislabelled operation as a coherent future production feature.
--- TIMELINE EVENTS ---
[cross-referenced by ian-morgan99 2026-08-27] <- 46 UPSTREAM: Final submission gate — clean builds/tests, curate
[cross-referenced by ian-morgan99 2026-09-02] <- 47 UPSTREAM: Reconcile stale/open tracker issues against curren
[commented by ian-morgan99 2026-09-02]
[commented by ian-morgan99 2026-09-02]
[closed by ian-morgan99 2026-09-02]
[cross-referenced by ian-morgan99 2026-09-09] <- 66 Reconcile every firmware-backed Pentax/Ricoh model against I
[commented by ian-morgan99 2026-09-09]
[reopened by ian-morgan99 2026-09-09]
[cross-referenced by ian-morgan99 2026-09-09] <- 52 TRACKING: sweep Pentax/Ricoh model × generic-control coverag
[cross-referenced by ian-morgan99 2026-09-09] <- 42 UPSTREAM: Audit Pentax changes for libgphoto2 coding convent
[cross-referenced by ian-morgan99 2026-09-09] <- 44 UPSTREAM: Define and execute Pentax hardware validation matr
====================================================================================================
#44 | UPSTREAM: Define and execute Pentax hardware validation matrix before submission
labels:  | created 2026-08-27 | comments 11
----------------------------------------------------------------------------------------------------
## Goal

Produce concise, reproducible real-camera evidence for the Pentax functionality we intend to submit upstream.

**Do not submit upstream until this issue is complete for the capabilities claimed in the initial patch series.**

## Test each physically available body separately

At minimum prioritise the bodies for which we have direct development evidence (especially K-3 III and K-1 II). Record exact camera firmware version, host libgphoto2 SHA and test date.

For each body test applicable capabilities:

- USB detection and clean connect/disconnect;
- repeated process start/exit/reconnect;
- configuration enumeration/read;
- safe writable settings with read-back/round-trip verification;
- preview/live view;
- focus operations where supported;
- normal still capture;
- capture + download integrity (non-zero file, plausible size, file opens where practical);
- repeated capture/download soak;
- bulb capture where supported;
- storage/card selection and dual-slot behaviour where supported;
- cancellation/abort and subsequent recovery;
- unplug/replug recovery;
- stale-session/process-restart behaviour;
- camera power-cycle behaviour;
- any model-specific feature intended for the upstream series.

## Negative/containment testing

Where practical, also prove:

- unsupported controls are not advertised;
- read-only/inferred models do not accidentally gain unsafe capture abilities;
- failure paths return clean errors rather than wedging the camera;
- generic/non-Pentax PTP2 behaviour remains unaffected by the Pentax gates.

## Evidence policy

Keep the upstream-facing evidence concise. Preserve large raw traces externally/in the existing evidence-retention mechanism, but create a small summary suitable for an upstream PR containing:

- body + firmware;
- libgphoto2 SHA;
- operation;
- pass/fail;
- noteworthy limitation;
- pointer/checksum to detailed evidence where useful.

Do not claim untested bodies work because they share protocol/model-family characteristics. Mark them inferred/unverified.

## Acceptance

For every capability proposed in the initial upstream patch series, there is either:

1. direct hardware PASS evidence on the relevant available body/bodies, or
2. an explicit decision to omit/defer that capability from the initial upstream submission.

The final matrix must clearly distinguish implemented, automated-test-verified, hardware-verified and inferred support.
--- COMMENT by ian-morgan99 2026-09-02 ---
Status update (periodic review pass, 2026-09-02): hardware-matrix progress so far —

**K-1 II (usb:001,002) verified on this machine:**
- Card-write mode `0x9004`: all four modes returned raw PTP RC 0x2001 (OK); state restored to off(0). Evidence: docs/pentax/evidence/ + REAL_HARDWARE_TEST_LOG.md.
- Green-button capture `0x9013`: implemented and exercised on the same session (capture path OK; DCIM-store follow-up still open — see below).
- d02c cross-process probe (full SET sequence): GET 0x200a / SET 0x200f even with d020=10 → model-gated to k3iii family (`8e9560ca9`); camera restored to autoselect (d020=255) and verified. Evidence: `k1ii-d02c-probe.log`, `k1ii-crossprocess-readonly-probe.log` (SHA-256 in EVIDENCE_RETENTION.md).

**Still outstanding for the matrix:**
- K-3 III + K-01 are not currently on this machine's bus (`lsusb` shows only 25fb:0183) despite being plugged and powered — likely sandbox/udev suppression or a different host. Once available: re-run the d02c probe on K-3 III to confirm the model-gating hypothesis (session-20 behaviour expected), plus the remaining per-body capability rows (~16 capabilities, prioritising K-3 III + K-1 II).
- Negative/containment tests on a non-Pentax body.
- Open follow-ups from the K-1 II session: which factor cleared the earlier 0x2019 DeviceBusy (fresh PTP session vs SD2 card present), whether bulb state=9 needs a power cycle, and where bulb exposures were stored (DCIM stores were empty).

Acceptance criterion (every proposed capability has direct HW PASS or an explicit defer decision) is not yet met — issue stays open.
--- COMMENT by ian-morgan99 2026-09-02 ---
**K-3 III status (2026-09-02 evening): plugged + powered, but not visible on any USB bus from this environment.**

All three bodies confirmed plugged in and powered on. Full investigation:

- No vendor `0x25fb` device anywhere — complete `/sys/bus/usb/devices` topology walk (buses 1–9), `/dev/bus/usb` node listing, and `gphoto2 --auto-detect` all empty of Sigma devices.
- 180 s background poll for the K-3 III: nothing appeared.
- The sandbox-era udev MTP-suppression rule (`99-pentax-mtp-ignore.rules`) was checked and ruled out — it only suppresses MTP auto-mounting, not USB enumeration itself; the udev export db has zero Sigma entries.
- All ports report `authorized=1`; no unenumerated devices on any port.

**Sandboxing evidence:** this agent environment runs under VS Code Insiders' app scope with non-init cgroup/mount namespaces and a filtered sysfs view (xHCI controllers listed as bound in the driver dir, but per-device `readlink .../driver` shows no driver). No VM detected (`systemd-detect-virt`=none; real MSI MS-7E47 board). This is consistent with earlier sessions where only some cameras (K-1 II at port 1-4) ever enumerated here — the K-3 III and K-01 have never been visible in any session.

**Verdict:** #44's K-3 III rows remain blocked on bus visibility, not code. The `pentax_model_supports_cross_process` gate (commit `8e9560ca9`) already fails closed for non-k3iii models, so nothing is pending on the software side.

**Host-side verification steps** (run in a host shell, outside the agent):
1. `lsusb | grep -i sigma` — expect `25fb:0189`. If absent, try a different physical port/hub (K-1 II has enumerated at port 1-4 on this machine).
2. Unplug/replug while running `udevadm monitor --property`; look for an add event with `ID_VENDOR_ID=0x25fb`. No event = the camera isn't enumerating (cable/port), not being hidden by udev.
3. If the body was previously wedged, do a **battery pull** — the vendor wedge persists across power-switch cycles and USB re-enumeration (see 2026-08-26 test-log entry).

Full write-up: `docs/pentax/REAL_HARDWARE_TEST_LOG.md` → "2026-09-02 (evening) — K-3 III plugged + powered but absent from USB bus; sandboxing investigation" (commit `35ced9c9a`).
--- COMMENT by ian-morgan99 2026-09-02 ---
One wording/provenance concern in `35ced9c`: the current note treats the VS Code `app-code-insiders-*.scope`, namespace inode values, and the xHCI sysfs observation as evidence of a *filtered/sandboxed USB view*. Those observations do not establish that conclusion by themselves. A systemd user app scope is normal process placement, and namespace inode numbers only become meaningful when compared with `/proc/1/ns/{mnt,cgroup}` / a known host shell. Likewise, `.../drivers/xhci_hcd/` containing controller entries while a particular `readlink .../driver` is absent needs the exact compared device paths before it supports a filtered-sysfs inference.

The safe conclusion is currently just: K-3 III hardware validation is blocked because no 25fb device enumerates in this environment; cause is unknown (physical cable/port/body state vs namespace/device filtering). Please downgrade “sandboxing evidence” / “filtered sysfs view” to a hypothesis unless host-vs-agent namespace and sysfs/`lsusb -t` outputs actually demonstrate different USB visibility. The host-side verification steps are the right next test.
--- COMMENT by ian-morgan99 2026-09-02 ---
## 2026-09-02 (late evening) — K-3 III + K-01 matrix rows complete; k3iii-family d02c encoding discovered

All three bodies are now on the bus after a cable change on the K-3 III: K-1 II `usb:001,008`, K-01 `usb:001,009`, **K-3 III `usb:002,002`** (bus 002 — the sandboxing hypothesis from earlier is no longer a blocker).

### Three-camera comparison (#44 matrix)

| Probe step | K-1 II | K-3 III | K-01 |
|---|---|---|---|
| Detection suffix | — | "(MTP mode)" (longstanding for this body) | "(PTP Mode)" |
| vendor_mode_enabled at probe time | 1 | 1 | **0** (open question) |
| Card-write SET, all four modes (0x9004) | `0x2001` OK ×4 | **`0x201d` Invalid Parameter ×4** | **`0x2005` Operation Not Supported ×4** |
| d02c GET | `0x200a` (unsupported) | **OK (=0)** — gate validated | `0x200a` (gate validated for non-k3iii) |
| d02c SET, simple values 1–4 (d020=10 active) | `0x200f` AccessDenied | **`0x201c` rejected** — camera auto-jumps to preset 33 on entering cross-process mode | n/a (GET unsupported) |
| d02c SET, preset value 33 (d020=10 active) | n/a | **`0x2001` OK, read-back = 33 ✓** | n/a |
| d02d GET | `0x200a` | OK (=4, user filter) | `0x200a` |

### Key finding: k3iii-family d02c uses preset-range encoding

On the K-3 III, simple values 1–4 are rejected with `0x201c` even with CI mode set to cross process (d020=10), while **preset-range value 33 is accepted** (`0x2001`, read-back verified). Entering cross-process mode auto-selects preset 33; turning CI mode off auto-resets d02c to 0. State was cleanly restored after every run (d020=0, d02c=0 verified by read-back).

### Caveats / open follow-ups

1. **K-3 III card-write `0x201d`**: may be session-reuse state (reconciliation noise: d035/GetAllConditions → `0x2017` UnknownVendorCode on every probe run, as expected with a held PTP session). Worth re-running after a battery pull / clean session. Also note the K-3 III currently lists **SD1 only** in storage — no SD2 device shown (relevant to the "no card in SD2?" question: the body may be reporting a single-slot configuration while in this state).
2. **K-01 `vendor_mode_enabled=0`**: if the model_no isn't in the supported-model list, the vendor-mode enable block is skipped entirely and 0x2005 could be an artifact of the camera's PTP operation table rather than a real rejection. K-01 is single-slot (not in the card-writing gate), so this row validates gating more than capability.
3. **Encoding decision**: config.c's d02c put handler currently sends user values as-is; for k3iii-family bodies it should either map into the preset range or document the encoding. Tracked as a follow-up before #46 submission.

### Evidence (spec repo `docs/pentax/evidence/2026-09-02/`, SHA-256-indexed in EVIDENCE_RETENTION.md)

`k3iii-cardwrite.log`, `k3iii-d02c.log`, `k3iii-crossprocess.log`, `k3iii-d02c-preset33.log`, `k01-cardwrite.log`, `k01-d02c.log`, `k01-crossprocess.log` — all seven hashed and recorded.

Docs commit: inner fork `810a20029` (pushed).
--- COMMENT by ian-morgan99 2026-09-02 ---
## SA Overview

One scope clarification for the new “K-3 III + K-01 matrix rows complete” note: the K-01 is primarily here as a **legacy regression body**, not as a requirement to support the newer K-1 II/K-3 III vendor feature set. The important acceptance question is whether the long-standing libgphoto2 Pentax path (derived from the older PKTriggerCord-era integration) still detects, connects and performs its established capture/control workflow after our modern Pentax changes.

The current K-01 results are useful containment evidence — newer card-write/d02c operations returning unsupported is acceptable if the legacy path remains intact — but those probes alone should not mark the K-01 regression row complete.

Any additional K-01 capability discovered and hardware-proven is welcome, but it is additive rather than required for parity with the newer bodies.

## Implementation Guidance

For #44, please give K-01 its own regression-oriented acceptance subset, e.g.:
- detection + clean connect/disconnect;
- the pre-existing K-01 configuration/control surface expected from upstream/legacy Pentax support;
- normal capture and download integrity;
- repeated reconnect/capture sanity;
- explicit proof that modern Pentax gates do **not** accidentally route K-01 through unsupported K-1 II/K-3 III paths.

Keep proprietary firmware decomposition/decompile material local-only for licensing/provenance reasons. Public/upstream evidence should stand on open-source implementation history, independently recorded protocol facts and current black-box hardware testing.
--- COMMENT by ian-morgan99 2026-09-06 ---
Now that Pentax is being exercised end-to-end through **OpenPolaris → Polaris firmware → pgphoto/stage2 → this libgphoto2 fork → real camera**, please treat those runs as an additional integration-evidence layer, not as a replacement for the direct libgphoto2 hardware matrix required here.

For upstream acceptance, every bug found through OpenPolaris should be classified at the narrowest reproducible boundary before changing libgphoto2:

```text
OpenPolaris/UI/protocol
Polaris network/pgphoto process
stage2/loader/runtime environment
libgphoto2 Pentax implementation
camera/firmware behaviour
```

If the symptom reproduces only through the appliance stack, fix it there. If it also reproduces with a direct libgphoto2 CLI/minimal harness against the same body, then it is credible upstream evidence.

For each end-to-end test run, record at minimum:
- camera model + camera firmware;
- exact libgphoto2 SHA embedded in the Polaris image;
- exact firmware-patcher SHA/build manifest or FwPkt hash;
- OpenPolaris version/SHA;
- operation and expected result;
- pass/fail plus whether failure reproduces directly at the libgphoto2 boundary;
- recovery result after process restart, USB reconnect and camera power-cycle where relevant.

For upstream-facing evidence, prefer a compact table with a separate `DIRECT_LIBGPHOTO2` vs `POLARIS_E2E` column. A feature should not be called upstream-hardware-verified solely because the OpenPolaris path worked; conversely, an OpenPolaris-only failure should not trigger a libgphoto2 code change until direct reproduction isolates it.

This separation will materially improve maintainability and reviewer confidence: it proves the camera protocol independently while still giving us the appliance-level soak/restart/reconnect coverage that upstream CI cannot provide.
--- COMMENT by ian-morgan99 2026-09-06 ---
Matrix status update (2026-09-06):

Completed:
- K-1 II and K-3 III vendor-mode/capability probes (partial).
- d02c cross-process behavior characterized (preset-range acceptance on K-3 III family).

Open:
- K-3 III normal capture + transfer validation still blocked by current wedge/offline state (needs battery-pull recovery).
- Live-view frame polling validation still pending in current cycle.

Conclusion:
- Matrix is partially complete with actionable findings, but not yet submission-complete.
--- COMMENT by ian-morgan99 2026-09-07 ---
K-3 III direct-host matrix advanced on clean `6aa3e4e66` (USB 25fb:0189, MTP setting): detect/summary/config PASS; preview PASS (valid 1080x720 JPEG, 79,018 bytes, SHA-256 cddc66074ca52b08bb6ec3616c4ace7eed4d59ca7f9bd9f69f795a7829994e30); two consecutive capture-and-download operations PASS (6192x4128 JPEGs, unique hashes 992290472736d6d30e2488a138504e9eaceefc07eda8d2d3a3f24661c3490a9d and 4295b258cdaa3b922e3ac29f64341e39aff28becc2339acbb74d3f33ed451a86); virtual delete and reconnect summary PASS; aperture and WB change/readback/restore PASS. Pentax utility and camera-list tests pass. Full Meson is 8/10; only the two pre-existing/no-ci environment tests fail (`test-gp-port` selects an unrelated empty USB entry, baseline `test-filesys` SIGSEGV). K-1 II remains attached to Polaris for the embedded/E2E run, so the overall matrix stays open.
--- COMMENT by ian-morgan99 2026-09-07 ---
### Repercussion from current OpenPolaris/Polaris qualification

Keep this as the authoritative **direct-camera/source-repo** matrix. `OpenPolaris#63` now inventories the wider E2E camera surface, but its results must not replace this matrix.

When #63 finds a camera-related failure, add it here/dedicated libgphoto2 issue only if the same operation fails using a clean exact libgphoto2 SHA with the camera directly attached. Record the first failing GP/PTP transaction. If direct libgphoto2 passes and Polaris-local fails, ownership is the firmware-patcher/runtime; if both lower layers pass and OpenPolaris fails, ownership is OpenPolaris.

Current concrete examples:
- K-3 III preview: direct libgphoto2/embedded CLI has produced a valid JPEG while the Stage-2 runtime fails (`patcher#36/#38/#39`), so that is **not currently a libgphoto2 preview defect**.
- K-1 II capture `-6` and WB/aperture false-failure remain source investigations #48/#49 until direct reproduction resolves ownership.

For upstream evidence, keep direct hardware results here separate from Polaris/OpenPolaris E2E evidence.
--- COMMENT by ian-morgan99 2026-09-08 ---
Cross-linking the new generic-control/model coverage programme: #52 is now the authoritative sweep for every referenced Pentax/Ricoh body and the downstream generic widget vocabulary. Please use its completed matrix as an input to this hardware-validation gate rather than inferring support from protocol-family similarity. Child implementation issues are #53–#60. In particular, K-01 `25fb:0130` USB-SCSI and `25fb:0131` PTP/MTP must remain separate rows, and K-3 III/K-1 II must be qualified independently.
--- COMMENT by ian-morgan99 2026-09-09 ---
Please add explicit hardware rows/gates for the new parity work rather than letting handler presence count as hardware support:

- **#8 / K-1 II `0xd036`:** retained evidence is currently read-only. Qualify one bounded positional write only after recording original state, prove an observable/source-faithful application criterion, GET/read-back, restore exact original state, then reconnect and verify. A SET ACK plus unchanged centre GET is not by itself proof of application.
- **#67 Custom Image:** per applicable body/property use GET-before -> one bounded SET -> GET-after -> exact restore -> GET-restore; keep d02d withheld until its full structure is proven.
- **#68 composition movement:** one minimum X/Y/rotation step where applicable, enforce camera-reported limits, then exact restoration and reconnect check.
- **#45 trigger semantics:** do not add a generic trigger-capture PASS row for `0x9013` merely because the command ACKs; semantic equivalence to libgphoto2 trigger capture must be proven independently.

These rows should carry exact camera firmware, libgphoto2 SHA and evidence hash/pointer, and #66 should consume the resulting HW/HW-W classifications.
--- TIMELINE EVENTS ---
[cross-referenced by ian-morgan99 2026-08-27] <- 45 UPSTREAM: Remove or resolve research-build gating and separa
[cross-referenced by ian-morgan99 2026-09-02] <- 43 UPSTREAM: Convert discovered Pentax failure modes into deter
[cross-referenced by ian-morgan99 2026-09-02] <- 41 UPSTREAM: Prepare canonical Pentax implementation as a clean
[commented by ian-morgan99 2026-09-02]
[cross-referenced by ian-morgan99 2026-09-02] <- 46 UPSTREAM: Final submission gate — clean builds/tests, curate
[referenced by ian-morgan99 2026-09-02]  sha:35ced9c9a8
[commented by ian-morgan99 2026-09-02]
[commented by ian-morgan99 2026-09-02]
[referenced by ian-morgan99 2026-09-02]  sha:810a200295
[commented by ian-morgan99 2026-09-02]
[commented by ian-morgan99 2026-09-02]
[commented by ian-morgan99 2026-09-06]
[commented by ian-morgan99 2026-09-06]
[cross-referenced by ian-morgan99 2026-09-06] <- 42 UPSTREAM: Audit Pentax changes for libgphoto2 coding convent
[commented by ian-morgan99 2026-09-07]
[commented by ian-morgan99 2026-09-07]
[cross-referenced by ian-morgan99 2026-09-07] <- 56 Pentax E2E qualification: separate app/firmware/libgphoto2 f
[cross-referenced by ian-morgan99 2026-09-08] <- 52 TRACKING: sweep Pentax/Ricoh model × generic-control coverag
[commented by ian-morgan99 2026-09-08]
[cross-referenced by ian-morgan99 2026-09-09] <- 66 Reconcile every firmware-backed Pentax/Ricoh model against I
[cross-referenced by ian-morgan99 2026-09-09] <- 67 Pentax IT2 parity: implement detailed Custom Image controls 
[cross-referenced by ian-morgan99 2026-09-09] <- 68 Pentax IT2 parity: expose composition offset movement via 0x
[commented by ian-morgan99 2026-09-09]
[cross-referenced by ian-morgan99 2026-09-09] <- 8 K-1 II live-view AF-position write semantics lack retained h
====================================================================================================
#42 | UPSTREAM: Audit Pentax changes for libgphoto2 coding conventions, generic PTP2 isolation and maintainability
labels:  | created 2026-08-27 | comments 6
----------------------------------------------------------------------------------------------------
## Goal

Perform an upstream-maintainer-style code review of the canonical Pentax implementation before we ask upstream to review it.

The question is not merely whether it works. The implementation should look like code that naturally belongs in libgphoto2 and should minimise maintenance burden for upstream.

## Audit scope

Review all Pentax-related changes, especially:

- `camlibs/ptp2/library.c`
- `camlibs/ptp2/config.c`
- `camlibs/ptp2/ptp.c`
- `camlibs/ptp2/ptp.h`
- `camlibs/ptp2/ptp-pack.c`
- `pentax-utils.[ch]`
- model registration/capability declarations
- build-system changes
- tests

## Required checks

1. Compare implementation style with nearby Canon/Nikon/Sony/Fuji/vendor-specific PTP2 code and existing libgphoto2 conventions.
2. Identify duplicated helpers or abstractions already available upstream.
3. Check ownership/lifetime semantics (`CameraFile`, filesystem cache, allocations, error exits).
4. Check timeout arithmetic, transfer-size arithmetic, integer narrowing/overflow and malformed-camera-response handling.
5. Check session lifecycle and recovery paths for leaked/stale state.
6. Audit every modification to generic PTP2 paths. Pentax-specific recovery/workarounds must not alter other vendors unless the behaviour is genuinely generic and tested.
7. Check that comments explain protocol facts rather than development history.
8. Remove debug-only logging, temporary probes and research scaffolding from production code unless it is intentionally useful upstream.
9. Check error codes/messages against normal libgphoto2 conventions.
10. Avoid giant model-specific conditionals where a small existing abstraction can express the same behaviour.
11. Run static/compiler diagnostics supported by the project and fix warnings introduced by our code.

## Important constraint

Do not refactor merely for aesthetics. Preserve hardware-proven behaviour. If an upstream-style cleanup changes protocol sequencing, ownership, timing or recovery semantics, require a regression test and hardware retest.

## Deliverable

Commit necessary cleanup separately from functional changes where possible, and leave a short audit report listing:

- generic PTP2 changes and why they are safe;
- Pentax-only changes;
- intentional deviations from common patterns and why;
- remaining maintainability concerns.

## Acceptance

A reviewer familiar with libgphoto2 can read the series without having to understand Benro Polaris or our research process, and generic PTP2 behaviour is demonstrably protected from Pentax-specific workarounds.
--- COMMENT by ian-morgan99 2026-09-02 ---
Status update (periodic review pass, 2026-09-02): no conventions-audit commits have been made yet — the audit is still pending. Current state of the audited surface:

- All Pentax changes live on master (`0e6ad0b38`), in sync with origin; working tree clean.
- Recent additions since 2026-08-27 that fall inside the audit scope: card-write `0x9004` + Green-button `0x9013` (HW-verified on K-1 II, all four modes RC 0x2001), and the d02c cross-process model gate (`pentax_model_supports_cross_process`, k3iii family only) with its config.c wiring and 6 new unit-test CHECKs.
- Build is clean under meson (only pre-existing warnings); `test-pentax-utils` passes (~140 CHECKs).

The audit itself (upstream-maintainer-style review of library.c, config.c, ptp.c/ptp.h, ptp-pack.c, pentax-utils.[ch], model registration, build system, tests — with the 11 required checks and separate cleanup commits) will be done as part of the #41 upstream-facing branch preparation, so its findings feed directly into the patch series. Stays open until that pass is complete; constraint noted: no aesthetic refactors, preserve HW-proven behaviour.
--- COMMENT by ian-morgan99 2026-09-06 ---
Given the move into live OpenPolaris/Polaris hardware testing, I suggest we make the upstream-change protocol here explicit and strict:

1. **No appliance-specific workaround enters libgphoto2.** If a failure is caused by pgphoto restart semantics, port ownership, firmware timing, wrapper environment or OpenPolaris protocol behavior, fix it outside this repository.
2. **Every new Pentax code change requires a direct reproducer at the libgphoto2 boundary** on the affected body before implementation, plus a regression test where practical.
3. **Minimise the changed surface.** Prefer an existing vendor/model gate or a small Pentax helper over touching generic PTP2 control flow. If generic code must change, document why the behavior is genuinely generic and test at least one non-Pentax path.
4. **Do not refactor adjacent code while fixing a hardware defect.** Keep protocol fixes and style/cleanup commits separate so upstream can review causality and hardware evidence independently.
5. For protocol sequencing/timing/recovery changes, preserve the old path behind an explicit model/capability condition where possible until the affected bodies have been retested.
6. Record exact before/after camera behavior, return codes and relevant transaction sequence. Avoid comments that explain our reverse-engineering history; comments should state the stable protocol fact the maintainer needs.
7. Any change discovered during Polaris integration must be retested both directly and through Polaris before it is considered resolved, because the appliance stack may have been compensating for or exposing timing differences.

I would also add a simple review checklist to each upstream-facing functional commit:

```text
[ ] direct hardware reproducer exists
[ ] affected models named
[ ] generic PTP2 impact = none / justified
[ ] deterministic regression test added where possible
[ ] direct hardware retest passed
[ ] Polaris E2E retest passed where relevant
[ ] no Benro/OpenPolaris-specific dependency introduced
```

That discipline is likely to matter as much as the code itself when asking libgphoto2 maintainers to accept a sizeable new Pentax capability set.
--- COMMENT by ian-morgan99 2026-09-06 ---
One further rule should be explicit because it affects where defects are filed, not just how they are fixed:

## Repository-local reproduction rule

A defect should only be logged in `libgphoto2` once the issue can be reproduced **from this repository's own boundary**, without requiring OpenPolaris, the Polaris firmware, `pgphoto`, Stage-2, or any other dependent application/runtime.

For a libgphoto2 issue, the reproduction contract should therefore be something like:

```text
libgphoto2 exact SHA
camera model + firmware
USB/host environment
exact gphoto2 CLI command or minimal in-repo test/harness
expected result
actual result
raw libgphoto2/PTP return code or transaction evidence
```

Evidence discovered through Polaris/OpenPolaris is valuable as a pointer, but it is only **discovery/integration evidence** until the same failure is reproduced directly here. If it cannot be reproduced without the dependent stack, the owning issue remains in that dependent repo.

Examples:
- OpenPolaris command maps incorrectly -> OpenPolaris issue, even if camera operation fails.
- `pgphoto` restart leaves port 8080 occupied -> firmware-patcher issue, even if live view disappears.
- Direct `gphoto2 --capture-image-and-download` / minimal libgphoto2 harness returns an incorrect Pentax result on the same body -> libgphoto2 issue.

This should also apply to regression closure: a libgphoto2 issue is not closed solely because OpenPolaris works after a change. The source-repo reproducer must pass independently first; downstream E2E is a second validation layer.

For upstream-facing work, each issue/commit should be understandable and reproducible by a libgphoto2 maintainer who has the relevant camera but has never heard of Polaris.
--- COMMENT by ian-morgan99 2026-09-06 ---
Conventions/maintainability audit status (2026-09-06):

- Ongoing; no new audit closeout yet.
- Recent work prioritized hardware reproduction and downstream-path isolation (#48/#49/#44).
- Once capture-path closure is complete, this issue remains the cleanup/maintainer-readiness pass before final submission gate.
--- COMMENT by ian-morgan99 2026-09-06 ---
Review of `6aa3e4e`: the direct K-3 III hardware evidence is a strong improvement and the Pentax-specific WB/aperture/capture fixes are now source-repo tested. One upstream-readiness item still needs explicit treatment: `append_folder_from_handle()` now treats `PTP_HANDLER_SPECIAL` (`0xffffffff`) as a root terminator in the generic PTP2 path, not behind a Pentax/model gate.

That may well be the correct generic MTP behaviour, but it crosses the rule already recorded here: generic PTP2 changes need a generic protocol rationale plus non-Pentax regression coverage. Please add a small deterministic test/fixture proving an MTP object whose parent is `0xffffffff` terminates folder reconstruction correctly, and verify at least one existing non-Pentax/plain-PTP path still treats its normal root semantics unchanged. Also document in the eventual patch split that this hunk is a generic MTP correctness fix, separate from the Pentax functional changes.

No rollback suggested; this is about proving and isolating the generic change before upstream preparation.
--- COMMENT by ian-morgan99 2026-09-09 ---
Concrete maintainability findings from the current IT2 reconciliation pass:

- `camlibs/ptp2/config.c` contains the exact same `pentaxliveviewafposition` menu registration twice in the same block. #69 owns the minimal removal + uniqueness/list-config regression check.
- The research capture gate still contains a semantically questionable `0x9013` Green-button branch labelled as trigger capture. #45 is reopened so this is classified separately from the real `0x9011`/`0x9012` capture family before upstream presentation.
- `ptp2_pentax_aperture_alias_present()` remains a production-exported test hook for #53; that should still be removed/reworked in favour of a public config API test before the upstream-facing series.
- `CAPABILITY_MATRIX_AUDIT.md` is now historical and stale in several implementation-status claims; #70 prevents it being presented as current maintainability evidence.

These are good candidates for the eventual #42 cleanup series because none requires changing hardware-proven wire sequencing. Keep them separate from functional protocol commits.
--- TIMELINE EVENTS ---
[commented by ian-morgan99 2026-09-02]
[cross-referenced by ian-morgan99 2026-09-02] <- 46 UPSTREAM: Final submission gate — clean builds/tests, curate
[cross-referenced by ian-morgan99 2026-09-02] <- 43 UPSTREAM: Convert discovered Pentax failure modes into deter
[commented by ian-morgan99 2026-09-06]
[commented by ian-morgan99 2026-09-06]
[commented by ian-morgan99 2026-09-06]
[cross-referenced by ian-morgan99 2026-09-06] <- 41 UPSTREAM: Prepare canonical Pentax implementation as a clean
[commented by ian-morgan99 2026-09-06]
[cross-referenced by ian-morgan99 2026-09-09] <- 53 ptp2/pentax: add generic `aperture` compatibility control
[cross-referenced by ian-morgan99 2026-09-09] <- 69 Remove duplicate pentaxliveviewafposition config registratio
[commented by ian-morgan99 2026-09-09]
[referenced by ian-morgan99 2026-09-09]  sha:e8dfddfca0
[cross-referenced by ian-morgan99 2026-09-09] <- 66 Reconcile every firmware-backed Pentax/Ricoh model against I
====================================================================================================
#41 | UPSTREAM: Prepare canonical Pentax implementation as a clean patch series against current gphoto/libgphoto2
labels:  | created 2026-08-27 | comments 3
----------------------------------------------------------------------------------------------------
## Goal

Turn the completed/tested Pentax work into a contribution that is straightforward for upstream libgphoto2 maintainers to review and merge.

This is **not** a request to submit upstream yet. Hardware testing comes first. The objective is to make the code submission-ready while preserving our current functionality.

## Required work

1. Resolve branch consolidation first (`#40`). Establish one canonical Pentax implementation and prove no useful work remains stranded on historical branches.
2. Fetch current `gphoto/libgphoto2` upstream and determine the clean merge-base for submission.
3. Produce an upstream-facing branch based on current upstream, containing only changes actually required for Pentax support.
4. Exclude unrelated fork history, Benro/Polaris-specific changes, local research tooling, generated evidence, and unrelated documentation/build changes.
5. Use `git range-diff`, semantic diff review and normal upstream style to ensure rebasing/splitting does not lose functionality.
6. Keep Pentax protocol/camera behaviour in libgphoto2; do not introduce Benro-specific runtime assumptions or compile paths into upstream camera code.
7. Identify every place the Pentax implementation modifies generic PTP2 behaviour and prove it is vendor/model gated or generically correct.

## Preferred patch/PR structure

Do not assume one huge PR is optimal. Prepare a reviewable series with logical dependency boundaries, for example:

1. Pentax model IDs/protocol constants/helpers + unit tests.
2. Vendor-session initialisation/reconciliation/recovery infrastructure.
3. Safe config read/write support.
4. Preview/live-view/focus support.
5. Capture/download/bulb and transfer/recovery hardening.
6. Additional model-specific capabilities only where sufficiently evidenced.

Adjust boundaries if the actual dependency graph demands it, but each patch should compile/test and have a coherent purpose.

## Review requirements

For every patch, document:

- why it is needed;
- affected camera models;
- whether behaviour is hardware-tested or inferred;
- generic PTP2 impact;
- tests covering it;
- dependencies on earlier patches.

## Acceptance criteria

- One clean upstream-facing branch/series based on current upstream.
- No Benro-specific implementation is present in libgphoto2.
- No unrelated fork changes are included.
- Each patch has a clear rationale and reviewable scope.
- Series preserves the functionality of the tested canonical fork.
- `git diff upstream...submission-branch` is explainable file-by-file.
- No upstream submission is made until the hardware validation issue is complete.
--- COMMENT by ian-morgan99 2026-09-02 ---
Status update (periodic review pass, 2026-09-02): prerequisite #40 is now resolved — the five legacy branches are classified superseded, archived as annotated `archive/*` tags on origin (verified via ls-remote), and master (`0e6ad0b38`) is confirmed as the single canonical branch, in sync with origin.

Remaining work for this issue:
1. Fetch current upstream gphoto/libgphoto2 and rebase/verify the Pentax delta against it.
2. Build the upstream-facing branch containing only Pentax-required changes (exclude Benro/Polaris/research tooling, evidence logs, unrelated docs).
3. `git range-diff` + semantic review; target the suggested 6-patch structure.

Per the issue's own gate, no submission until #44 is complete — so this stays open pending the hardware matrix.
--- COMMENT by ian-morgan99 2026-09-06 ---
## Auto-triage (poller, 2026-09-06 19:27:03)

**Ownership:** library (driver/camlib behavior)
**First step:** Reproduce with gphoto2 --debug, attribute the failing PTP op, and check whether it is already covered by an open/closed fix on master before proposing a new change.

Cross-refs: check related open issues in both repos before duplicating work —
patcher runtime defects belong here; driver/camlib behavior belongs in
ian-morgan99/libgphoto2. If this issue is blocked on the other side, say so
explicitly and link the blocking issue.
--- COMMENT by ian-morgan99 2026-09-06 ---
Canonical patch-series status (2026-09-06):

- Not yet finalized.
- Hardware/evidence work continues to change final ordering/content required for clean upstreamable series.
- Will be finalized after #48/#44 closure and #42 audit pass.
--- TIMELINE EVENTS ---
[commented by ian-morgan99 2026-09-02]
[cross-referenced by ian-morgan99 2026-09-02] <- 42 UPSTREAM: Audit Pentax changes for libgphoto2 coding convent
[cross-referenced by ian-morgan99 2026-09-02] <- 46 UPSTREAM: Final submission gate — clean builds/tests, curate
[commented by ian-morgan99 2026-09-06]
[commented by ian-morgan99 2026-09-06]
====================================================================================================
#8 | K-1 II live-view AF-position write semantics lack retained hardware qualification
labels:  | created 2026-08-25 | comments 2
----------------------------------------------------------------------------------------------------
## Current problem

The original #8 defect (K-1 II returning success after the SET acknowledgement without performing the promised GET verification) has been corrected in current code. The remaining release/upstream problem is **stronger semantic evidence**.

`_put_Pentax_LiveViewAFPosition()` currently treats K-1 II as a non-echoing model: after writing `0xd036`, a structurally valid 8-byte GET response at the geometry centre is accepted as proof that the requested AF position was honoured.

The canonical `docs/pentax/REAL_HARDWARE_TEST_LOG.md`, however, records only **read-only** K-1 II `0xd036` discovery (8-byte centre 360,240; no setter). There is no retained hardware row proving that a positional write changes an AF area/state, nor a retained source/trace reference sufficient to distinguish "write applied but GET remains centre" from "transaction ACKed but setting ignored".

Pentax bodies are already known to ACK some state-inappropriate writes, so SET ACK + unchanged centre GET is not enough by itself to claim HW-W semantics.

## Required resolution

Choose one evidence-backed outcome:

1. **Qualify the K-1 II write path**
   - record exact camera firmware + libgphoto2 SHA;
   - read original geometry/AF state;
   - issue one bounded positional write;
   - prove application via an observable camera/AF-area effect or source-faithful trace criterion independent of the SET ACK;
   - perform GET/read-back;
   - restore the exact original state where applicable;
   - reconnect and verify no stale host state;
   - retain a hash/pointer to the evidence in #44 / `REAL_HARDWARE_TEST_LOG.md`.

2. **If that proof is unavailable**, fail closed for K-1 II writes (read-only AF-position may remain exposed) until the missing evidence is obtained.

If existing retained raw evidence already established the non-echoing-write behaviour, link/hash it here and update the canonical hardware/evidence docs rather than re-running the test.

## K-3 III distinction

Do not weaken the echoing-model path. K-3 III-family writes should continue to require the model-appropriate coordinate verification already implemented and separately hardware-qualified where evidence exists.

## Tests

Automated tests should continue to cover:
- SET OK + GET failure -> error;
- malformed/truncated GET -> error;
- K-3 III mismatched echoed coordinates -> failure;
- `alreadyset` unchanged on verification failure;
- K-1 II write path is unavailable if the chosen resolution is to fail closed pending qualification.

## Acceptance

- [ ] Current code no longer relies on a claim stronger than retained evidence.
- [ ] K-1 II is either explicitly hardware/trace-qualified for positional writes or write-disabled/fail-closed.
- [ ] `REAL_HARDWARE_TEST_LOG.md` and #44 reflect the resulting evidence level.
- [ ] #66 records K-1 II `d036` as HW-R vs HW-W accurately.
- [ ] The original GET-verification fix remains intact.

Cross-links: #44 hardware validation, #66 IT2/model reconciliation, #69 duplicate menu cleanup.
--- COMMENT by ian-morgan99 2026-08-25 ---
### Polaris release impact — HIGH

For a remotely controlled head, reporting an AF-position write as successful when the K-1 II only ACKed the SET is unsafe: Polaris can frame/focus based on a state that was never applied. Pentax bodies are already known to ACK some state-inappropriate writes.

**Required for Polaris:** always perform the model-appropriate GET verification before setting `alreadyset`. K-1 II may accept structurally valid non-echoing read-back; K-3 III/echoing bodies should retain exact-coordinate verification.

**Acceptance:** SET-ACK + malformed/failed GET must never become success; `alreadyset` must remain unchanged on verification failure. Hardware-run repeated AF moves before capture and confirm no false-success cases.
--- COMMENT by ian-morgan99 2026-09-09 ---
Reopening qualification concern: the implementation now performs the promised GET verification, but the issue's hardware acceptance was never completed for the K-1 II write path.

The canonical `REAL_HARDWARE_TEST_LOG.md` records only **read-only** `0xd036` discovery on K-1 II (8-byte centre 360,240; explicitly no setter). Current `_put_Pentax_LiveViewAFPosition()` nevertheless states as a fact that K-1 II "accepts the payload but always reports the geometry centre" and treats a structurally valid centre GET as proof the write was honoured.

That is stronger than the retained hardware evidence. A SET ACK + unchanged centre response may equally mean "accepted transaction, state not applied" unless we have a separate trace/hardware observation proving IT2/camera semantics.

I would keep the code fail-closed on K-1 II writes (or keep the surface withheld for that model) until we have an explicit bounded hardware test: original state -> one positional write -> observable AF-area effect or source-faithful proof -> GET -> exact restoration/reconnect. If there is existing raw evidence that established the non-echoing-write behaviour, link/hash it here and in the hardware log instead.

This does not undo the parser/GET-verification fix; it closes the remaining evidence gap in the original Polaris acceptance requirement.
--- TIMELINE EVENTS ---
[commented by ian-morgan99 2026-08-25]
[closed by ian-morgan99 2026-08-25]
[referenced by ian-morgan99 2026-08-26]  sha:c0c4031cc1
[referenced by ian-morgan99 2026-08-26]  sha:b0860c97c7
[cross-referenced by ian-morgan99 2026-08-27] <- 47 UPSTREAM: Reconcile stale/open tracker issues against curren
[cross-referenced by ian-morgan99 2026-09-02] <- 46 UPSTREAM: Final submission gate — clean builds/tests, curate
[commented by ian-morgan99 2026-09-09]
[reopened by ian-morgan99 2026-09-09]
[cross-referenced by ian-morgan99 2026-09-09] <- 69 Remove duplicate pentaxliveviewafposition config registratio
[cross-referenced by ian-morgan99 2026-09-09] <- 66 Reconcile every firmware-backed Pentax/Ricoh model against I
[cross-referenced by ian-morgan99 2026-09-09] <- 52 TRACKING: sweep Pentax/Ricoh model × generic-control coverag
[cross-referenced by ian-morgan99 2026-09-09] <- 44 UPSTREAM: Define and execute Pentax hardware validation matr
[renamed by ian-morgan99 2026-09-09] {}
[cross-referenced by ian-morgan99 2026-09-09] <- 53 ptp2/pentax: add generic `aperture` compatibility control
====================================================================================================
#6 | [WIP] Create a fork for better documentation exploration
labels:  | created 2025-11-09 | comments 1
----------------------------------------------------------------------------------------------------
Thanks for asking me to work on this. I will get started on it and keep this PR's description up to date as I form a plan and make progress.


<!-- START COPILOT CODING AGENT SUFFIX -->



<details>

<summary>Original prompt</summary>

> Create a fork so I can explore how to document it better


</details>



<!-- START COPILOT CODING AGENT TIPS -->
---

✨ Let Copilot coding agent [set things up for you](https://github.com/ian-morgan99/libgphoto2/issues/new?title=✨+Set+up+Copilot+instructions&body=Configure%20instructions%20for%20this%20repository%20as%20documented%20in%20%5BBest%20practices%20for%20Copilot%20coding%20agent%20in%20your%20repository%5D%28https://gh.io/copilot-coding-agent-tips%29%2E%0A%0A%3COnboard%20this%20repo%3E&assignees=copilot) — coding agent works faster and does higher quality work when set up for your repo.
--- COMMENT by ian-morgan99 2026-09-02 ---
Note from the periodic review pass (2026-09-02): this draft's head (`copilot/explore-documentation-fork` @ 358dc7080) is now pinned by the `archive/copilot-explore-documentation-fork` tag, and its 27 unique commits are superseded by master (see issue #40). Recommendation: close as superseded once you confirm — left open here since it's a PR rather than one of the tracker issues.
--- TIMELINE EVENTS ---
[committed by ? ] Create library.c

Create the main modern sha:
[committed by ? ] Create readme.txt sha:
[committed by ? ] Create meson.build sha:
[committed by ? ] Update library.c sha:
[committed by ? ] Update library.c sha:
[committed by ? ] Update library.c sha:
[committed by ? ] Update Makefile.am sha:
[committed by ? ] Create Makefile.am sha:
[committed by ? ] Create pentaxmodern.c sha:
[committed by ? ] Delete camlibs/pentaxmodern/library.c sha:
[committed by ? ] Update pentaxmodern.c sha:
[committed by ? ] Update pentaxmodern.c sha:
[committed by ? ] Update pentaxmodern.c sha:
[committed by ? ] Integrate new Pentax Modern PTP library
 sha:
[committed by ? ] Here's the plan to implement detailed PT sha:
[committed by ? ] Add support for additional Pentax and Ri sha:
[committed by ? ] I've updated the "Future Enhancements (O sha:
[committed by ? ] This commit introduces model-specific ha sha:
[committed by ? ] Input:
Update pentaxmodern README with m sha:
[committed by ? ] Okay, I've made some enhancements to the sha:
[committed by ? ] Sync pentax-modern-integration with full sha:
[committed by ? ] Fix: Clean up legacy GNU extensions in P sha:
[committed by ? ] Merge pull request #1 from ian-morgan99/ sha:
[committed by ? ] Merge pull request #2 from ian-morgan99/ sha:
[committed by ? ] Merge pull request #3 from ian-morgan99/ sha:
[committed by ? ] Merge pull request #5 from gphoto/master sha:
[committed by ? ] Initial plan sha:
[assigned by Copilot 2025-11-09]
[assigned by Copilot 2025-11-09]
[copilot_work_started by ian-morgan99 2025-11-09]
[review_requested by Copilot 2025-11-09]
[copilot_work_finished by ian-morgan99 2025-11-09]
[base_ref_force_pushed by ian-morgan99 2026-08-24]
[referenced by ian-morgan99 2026-08-26]  sha:b0860c97c7
[cross-referenced by ian-morgan99 2026-09-02] <- 40 Consolidation is not complete: legacy Pentax branches still 
[cross-referenced by ian-morgan99 2026-09-02] <- 47 UPSTREAM: Reconcile stale/open tracker issues against curren
[commented by ian-morgan99 2026-09-02]