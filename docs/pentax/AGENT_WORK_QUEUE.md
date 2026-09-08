# Pentax/Ricoh compatibility agent work queue

Revision: 2026-09-08

This file exists to let parallel agents work overnight without duplicating or conflating control semantics.

## Umbrella

- #52 — authoritative Pentax/Ricoh model × generic-control sweep
- #51 — discovery/index thread for pgphoto/libgphoto2 vocabulary mismatch

## Independent control work

- #53 — `aperture`
- #54 — `imageformat`
- #55 — `imagequality`
- follow-on child issues: `autofocus`, `autofocusdrive`, `manualfocus`, `manualfocusdrive`, `capturetarget`

## Rules for every agent

1. Read `docs/pentax/GENERIC_CONTROL_COMPATIBILITY_AUDIT.md` first.
2. Preserve existing Pentax-specific controls; add compatibility wrappers rather than destructive renames.
3. Do not assume K-3 III semantics apply to K-1 II or K-01.
4. Keep K-01 `25fb:0130` USB-SCSI and `25fb:0131` PTP/MTP as separate capability rows.
5. Do not map `pentaxcardwritingmode` to `imageformat`.
6. Keep `autofocus`, `autofocusdrive`, and `manualfocusdrive` separate.
7. Require model-aware focus dispatch (`0x9017` new family vs `0x9016` old family).
8. Add tests that prove value/action semantics, not merely that a widget name exists.
9. Record exact source SHA and hardware evidence; do not promote family inference to hardware support.
10. Update #52 matrix status when a control issue changes model coverage.

Detailed provenance-sensitive research may be recorded in private `ian-morgan99/PrivateResearch`; public code/issues should contain enough protocol facts and tests to review the implementation independently.