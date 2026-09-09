# Pentax/Ricoh compatibility agent work queue

Revision: 2026-09-09

This file exists to let parallel agents work without duplicating or conflating control semantics.

## Umbrella / sequencing

- #52 — authoritative Pentax/Ricoh model × control sweep
- #51 — discovery/index thread for pgphoto/libgphoto2 vocabulary mismatch
- #66 — durable model × USB-personality × IT2-capability reconciliation; may start from code/IT2 evidence now, but must consume firmware-derived findings from `ian-morgan99/benro-polaris-firmware-patcher#48` before completion

## Independent generic-control work

- #53 — `aperture`
- #54 — `imageformat`
- #55 — `imagequality`
- #56 — `autofocus` capture policy
- #57 — `autofocusdrive` immediate AF action
- #58 — `manualfocus` AF/MF mode semantics
- #59 — `manualfocusdrive` model-aware lens movement
- #60 — `capturetarget` semantics / graceful absence

Issues #61–#65 are closed duplicates; do not use them.

## IT2-parity / correctness work discovered by the sweep

- #67 — detailed Custom Image controls `0xd021`–`0xd029` and structured `0xd02d`
- #68 — composition-offset movement via `0x9018`; do not confuse with the existing `0xd02a` enable switch
- #69 — remove duplicate `pentaxliveviewafposition` config registration
- #70 — mark the 2026-08-21 `CAPABILITY_MATRIX_AUDIT.md` as historical/superseded for current status; preserve it for provenance
- #8 — **reopened**: K-1 II `0xd036` write semantics are not yet backed by retained write hardware evidence; do not treat a SET ACK plus centre GET as proof of application until qualified or trace-proven
- #45 — **reopened**: separate research `0x9013` Green-button semantics from real `0x9011`/`0x9012` capture before upstream promotion

## Upstream / hardware gates

- #42 — upstream conventions and maintainability audit
- #44 — per-body hardware validation matrix

## Rules for every agent

1. Read `docs/pentax/GENERIC_CONTROL_COMPATIBILITY_AUDIT.md`, `IMAGE_TRANSMITTER_CAPABILITY_MATRIX.md`, and this queue before coding. Treat `CAPABILITY_MATRIX_AUDIT.md` as a historical snapshot, not current status, until #70 is complete.
2. Preserve existing Pentax-specific controls; add compatibility wrappers rather than destructive renames.
3. Do not assume K-3 III semantics apply to K-1 II or K-01.
4. Keep K-01 `25fb:0130` USB-SCSI and `25fb:0131` PTP/MTP as separate capability rows; likewise K-1 II `0182` MSC and `0183` PTP.
5. Do not map `pentaxcardwritingmode` to `imageformat` and do not manufacture `capturetarget`.
6. Keep `autofocus`, `autofocusdrive`, positional AF (`0xd036`), and `manualfocusdrive` separate.
7. Require model-aware focus dispatch (`0x9017` new family vs `0x9016` old family); immediate AF must not be implemented by reusing either manual-focus family.
8. `0x9013` is not to be labelled as ordinary capture merely because it ACKs; exact semantic equivalence requires source/trace/hardware evidence.
9. Add tests that prove value/action semantics, not merely that a widget name exists. Avoid test-only production exports where the public API can be exercised instead.
10. Record exact source SHA and hardware evidence; do not promote family inference, firmware strings, or SET ACKs to hardware support.
11. Update #66 and #52 when a child changes model coverage or evidence status.
12. Keep proprietary/raw reverse-engineering artefacts in `ian-morgan99/PrivateResearch`; public code/issues should contain only derived protocol facts and enough evidence references to review the implementation independently.
