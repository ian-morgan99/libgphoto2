# Pentax/Ricoh generic-control issue index

Umbrella: #52
Discovery: #51

Canonical child issues:
- #53 `aperture`
- #54 `imageformat`
- #55 `imagequality`
- #56 `autofocus` capture policy
- #57 `autofocusdrive` immediate AF action
- #58 `manualfocus` AF/MF mode semantics
- #59 `manualfocusdrive` model-aware lens drive
- #60 `capturetarget` semantics / graceful absence

Issues #61–#65 were accidentally created during the same setup window after #56–#60 already existed; they are closed as duplicates and must not be used for implementation.

A child closes only when its own model gates, semantic tests and hardware acceptance criteria are met. Closing one control never implies the rest of #52 is complete.

Controls already exposed generically (`iso`, `shutterspeed`, `whitebalance`, `exposurecompensation`, and applicable `bulb` behaviour) remain explicit verification columns in #52. Create an additional child only when the sweep finds a concrete model-specific defect or semantic gap; do not create tickets merely to mirror rows that are already proven correct.