# Pentax/Ricoh control and IT2-parity issue index

Umbrella: #52
Discovery: #51

## Generic-control compatibility children

- #53 `aperture`
- #54 `imageformat`
- #55 `imagequality`
- #56 `autofocus` capture policy
- #57 `autofocusdrive` immediate AF action
- #58 `manualfocus` AF/MF mode semantics
- #59 `manualfocusdrive` model-aware lens drive
- #60 `capturetarget` semantics / graceful absence

Issues #61–#65 were accidentally created during the same setup window after #56–#60 already existed; they are closed as duplicates and must not be used for implementation.

## Full IT2-parity / model-reconciliation work

- #66 authoritative model × USB-personality × capability reconciliation; consumes firmware findings from `benro-polaris-firmware-patcher#48`
- #67 detailed Custom Image controls (`0xd021`–`0xd029`, `0xd02d`)
- #68 composition-offset movement via `0x9018` (distinct from the existing `0xd02a` enable switch)
- #69 duplicate `pentaxliveviewafposition` config registration cleanup
- #70 completed: `CAPABILITY_MATRIX_AUDIT.md` is explicitly historical/provenance-only; #66 owns current reconciliation

## Reopened evidence / upstream-readiness issues relevant to this programme

- #8 K-1 II `0xd036` live-view AF-position write qualification: parser/GET verification exists, but retained hardware evidence is still read-only; do not claim K-1 II write semantics until explicitly qualified
- #45 research-build gating: reopened because the `0x9013` Green-button branch must be classified separately from true `0x9011`/`0x9012` capture semantics before upstream promotion
- #42 upstream coding-conventions / maintainability audit
- #44 hardware validation matrix

A child closes only when its own model gates, semantic tests and hardware acceptance criteria are met. Closing one control never implies the rest of #52 is complete.

Controls already exposed generically (`iso`, `shutterspeed`, `whitebalance`, `exposurecompensation`, and applicable `bulb` behaviour) remain explicit verification columns in #52. Create an additional child only when the sweep finds a concrete model-specific defect or semantic gap; do not create tickets merely to mirror rows that are already proven correct.
