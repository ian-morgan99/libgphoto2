# Retrospective audit against the IMAGE Transmitter target

Revision: 2026-08-21

This audits all Pentax work completed before and during consolidation of
`IMAGE_TRANSMITTER_CAPABILITY_MATRIX.md`. A successful isolated test remains
valid evidence, but it does not close a matrix tier when an earlier prerequisite
is open.

## Findings and corrections

| Severity | Finding | Disposition |
|---|---|---|
| Critical | The public minimum-focus implementation always used the K-3 III new-focus algorithm/opcode `0x9017`. IT2 marks K-1 II as old-focus `0x9016`. | Fixed: getter and setter now reject every model except K-3 III; regression checks cover K-1 II fail-closed behavior. No K-1 II focus command had been issued. |
| High | K-1 II’s sparse DeviceInfo list was initially interpreted as a capability boundary. IT2 directly requests omitted descriptors. | Corrected in plan, capability summary, and H1.8. Tier 3 now requires guarded direct reads from the IT2 table. |
| High | The driver has identity and extension state but not a full runtime model-capability structure. Shared code could otherwise inherit K-3 III-only features. | New focus is now explicitly guarded. All other advanced rows remain unimplemented/withheld until model flags are represented and tested. |
| High | Past K-3 III preview/focus/ISO work and K-1 II DNG work occurred before the new 10+10 lifecycle tier was closed. | Results remain valid bounded evidence; none closes later acceptance tiers or authorizes public enablement. |
| Medium | `0xd036` and `0xd037` constants were named as zoom/mode, but IT2 uses AF position/zoom. | Corrected before either constant had a call site; no hardware impact. |
| Medium | Several IT2-named operations and direct properties were absent from the source vocabulary. | Constants added only. No wrappers, widgets, advertising, or hardware calls were added. Untraced named operations remain withheld. |
| Medium | Current `GetAllConditions` parsing covers only the first operational subset, not the complete IT2 state surface. | Safe partial parser retained. Remaining offsets are explicit matrix targets with field-specific length gates. |
| Medium | The K-1 II old-focus source path multiplies the requested direction by 5 and uses amount/direction parameters, unlike K-3 III’s open-aperture calculation. | K-1 II focus remains unavailable. Implement a separate bounded old-focus path; never reuse the K-3 III displacement helper. |
| Low | Existing documentation was fragmented and allowed the short capability summary to be mistaken for the target specification. | Normative matrix created and linked from the plan and summary. |

## Implementation audit

| Surface | What is correct now | What remains open |
|---|---|---|
| Identity | Exact VID:PID plus exact DeviceInfo string; K-1 II 78400 and K-3 III 78420; Monochrome excluded | Add an explicit capability structure before advanced shared implementations |
| Vendor lifecycle | Correct five parameters; Param2 flags; rollback after refresh failure; cleanup attempts disable | 10 cold + 10 warm per body; readiness/recovery behavior; official trace |
| DeviceInfo refresh | Atomic refresh after enable is safe and necessary for K-3 III | It cannot discover K-1 II’s omitted direct properties; add guarded direct descriptor inventory |
| Conditions | 508-byte mandatory bound; 532-byte optional Astro bound; raw unknowns; no setter | Complete safely gated status fields; per-mode correlation; tests for every new boundary |
| Live view | Original `0xd035` byte retained/restored; JPEG bounds; cleanup error preserved | K-1 II first frame; K-3 III 500/cold gate; geometry and AF/zoom rows |
| Focus | K-3 III source formula, one command, no retry escalation; physical Near/Far pass | K-1 II old `0x9016`; lifecycle-tier prerequisite; no public generalization |
| Capture | Correct five-parameter initiation; candidate polling; bounded grammar; cache before `0x900e`; cancellation/timeouts; default compile containment | All real vendor capture/transfer tests; candidate-handle special states; recovery after each failure phase |
| Generic files | K-1 II DNG selected by operator-created folder, hashed, EXIF checked, card untouched | Repeat DNG hash and add JPEG; K-3 III selected JPEG/RAW hashes |
| Configuration | Existing typed K-3 III descriptor reads; K-3 III ISO exact restore; source property symbols complete | K-1 II direct reads; model flags; compound `0xd01b` preservation; all other writes withheld |
| Bulb/Astro | K-1 II B=raw 9 and 30-second timer read; K-3 III B snapshots; safe status fields | No exposure, timer write, cancellation, Astro phase, or recovery claim |
| Polaris | Source-path packaging and policy tests exist | Exact-fork firmware/QEMU/device workflow remains blocked and cannot outrun camera tiers |

### Polaris repository reconciliation

The patcher accepts a local `--libgphoto2-source`, records its Git commit and
dirty-content hash, packages corresponding source, and rejects dirty input by
default. Its storage and capture-target compatibility shims are exact-name
Canon R5 II gates; the policy test confirms both Pentax target names are pure
pass-through. The policy test passed again during this audit.

The patcher still contained the original 621-line capability extraction. That
copy included the superseded K-3 III `0x018f` identifier and incorrect K-1 II
flags. Since an archived copy already exists in the workspace archive, the
active patcher document was replaced with a pointer to the normative matrix.
This removes a conflicting implementation target without discarding history.

The ARM loader compile was not rerun locally because the cross-compiler is not
installed on the host; its prior repository test remains recorded. Firmware
loader/QEMU/repack and actual Polaris testing remain open and require the user’s
FwPkt plus closure of the corresponding camera tiers.

## Hardware evidence reclassification

| Evidence | Still proves | Does not prove |
|---|---|---|
| K-3 III 3 cold + 3 warm sessions | Initial lifecycle repeatability under recorded readiness | 10+10 acceptance, arbitrary restart recovery, or wake-up |
| K-3 III 50 preview frames | Bounded warm-session frame transport and cleanup | K-1 II preview, 500 frames, cold recovery, public enablement |
| K-3 III `+23/-23` focus | New-path response and qualitative direction on that lens/body | K-1 II focus, calibration, larger steps, retry escalation |
| K-3 III ISO 3200→1600→3200 | `0xd01e` identity and exact restoration for those values/mode | General ISO setter or K-1 II write safety |
| K-1 II condition snapshots | 568-byte layout compatibility and recorded state fields | Complete capability enumeration or writable settings |
| K-1 II B snapshots | Raw mode 9, ISO 200, 30-second Bulb timer, stable read-only session | Bulb exposure/termination or direct shutter descriptor |
| K-1 II manual DNG | Generic standard-object download and ISO/EXIF correlation | Vendor remote capture/candidate transfer or repeatability |

## Test audit

Current focused fixtures correctly cover exact model matching, unsupported
identities, condition minimum/optional boundaries, JPEG framing, candidate
filename rejection, transfer write/seek grammar, cancellation/timeouts, and
capture containment. The new model-focus regression confirms only K-3 III can
enter the `0x9017` helper path.

After the corrections, a fresh isolated ptp2/libusb1 build completed and both
`test-pentax-utils` and `test-camera-list` passed with the build-tree camlib and
I/O-library paths. The build container had no camera device attached.

Missing fixtures, in target order:

1. model-capability table expectations for every K-1 II/K-3 III flag;
2. direct-property applicability and expected datatype table;
3. descriptor-response fixtures for K-1 II omitted properties;
4. complete condition-field boundary fixtures before extending the parser;
5. `0xd01b` 10/11-byte decode and byte-preserving mutation fixtures;
6. K-1 II old-focus amount/direction calculation and model dispatch;
7. live-view geometry/AF-position/zoom structures;
8. candidate-handle 1, preview type 253, and every transfer failure phase;
9. model-gated ability tests as features graduate from withheld state.

## Audit conclusion

Mechanical source reconciliation found 35 distinct direct property codes, 21
vendor operation codes with active executor call sites, and 25 named vendor
operation codes overall. Every one now has an explicit matrix row; grouped
ranges are not used as a substitute for individual coverage.

The safe core—identity containment, bounded condition subset, default capture
containment, K-3 III preview restoration, and transfer host bounds—remains
sound. The project is not capability-complete. Its largest functional gap is
the absence of IT2-driven direct K-1 II descriptor discovery; its largest
prevented safety defect was cross-model use of `0x9017`. The matrix tiers are
now the acceptance authority, and later evidence cannot silently close an open
earlier tier.
