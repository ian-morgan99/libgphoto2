# B1 Polaris candidate-source record — 2026-08-21

Status: **PARTIAL PASS; ARM/firmware dry run BLOCKED**

Polaris commit: `6210741` (`Fail closed for unverifiable candidate source`)

Implemented and exercised without firmware:

- clean read-only Git checkout records exact `HEAD` and no dirty hash;
- dirty checkout is rejected unless explicit opt-in is supplied;
- opted-in dirty content produces the same non-empty hash across two runs;
- source tar archive records its SHA-256;
- absolute/traversal/device/FIFO/unsafe-link archive members are rejected before
  extraction, and exactly one top-level source directory is required;
- explicit release selection and local-source selection are mutually exclusive;
- incompatible source version is rejected;
- source-input tests are executable through
  `container/test_source_input.sh IMAGE CLEAN_CHECKOUT`;
- candidate builds require the linked `ptp2.so` to retain the Pentax marker;
- candidate ptp2-only QEMU self-test failures are fatal;
- output includes `build-source-provenance.txt` and exact post-transformation
  LGPL corresponding source.

Observed test results:

```text
clean-provenance=PASS
dirty-rejection=PASS
dirty-opt-in=PASS
deterministic-dirty-hash=PASS
archive-provenance=PASS
safe-archive=PASS
unsafe-archive-rejection=PASS
version-rejection=PASS
mutual-exclusion=PASS
source-input preflight tests: PASS
```

No stock Polaris `FwPkt` was found in the workspace or attached external
storage. Therefore ARM linking, candidate-marker inspection of the resulting ARM
camlib, QEMU load, ABI checks, UBIFS repack, package allowlist, and reversible
stage2 output remain BLOCKED. Full-mode QEMU needs a test that loads the fresh
core rather than the existing legacy-core self-test.

