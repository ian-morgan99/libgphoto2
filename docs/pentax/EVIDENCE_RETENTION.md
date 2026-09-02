# Pentax evidence retention policy

Revision: 2026-08-26

This policy governs hardware-capture evidence stored under `docs/pentax/evidence/`.

## Goals

- Keep the upstreamable libgphoto2 diff dominated by source, tests, and
  documentation rather than large generated debug captures.
- Preserve reproducible, auditable protocol evidence without inflating
  repository history.
- Prevent accidental commits of very large raw traces or host-specific
  diagnostic content.

## Rules

1. **In-tree evidence** must be small and curated: human-readable traces,
   session summaries, and cross-check notes that directly prove a protocol
   fact. Keep individual files well under 1 MB; compress anything larger
   (`.gz`) and justify it in a commit message.
2. **Large raw captures** (multi-megabyte logs) must not be committed as
   plain text. Store them compressed out-of-tree (release/CI artifacts, a
   dedicated evidence repository, or another durable external archive) and
   reference them from documentation.
3. **Provenance**: every retained trace must be accompanied by camera model,
   firmware version, test command, date, relevant commit SHA, and SHA-256
   checksum so it remains reproducible and auditable. Record these in the
   sibling `.md` summary for that capture date.
4. **Privacy review**: before committing any trace, check it for incidental
   machine-specific paths, serial numbers, USB topology, or other diagnostic
   content that should not become permanent project history.
5. **Size guard**: do not add new `.log` files above 1 MB to git without an
   explicit decision recorded in this directory. Prefer compressing first.

## Current inventory (2026-08-26)

| File | Size | Status |
|---|---|---|
| `k1ii-bulb30s-capture.log` | ~171 MB | **untracked**, local only; never commit |
| `k1ii-bulb30s-capture.log.gz` | ~7 MB | tracked (compressed, justified) |
| `2026-08-25/k3iii-bulb30s-capture-debug.log.gz` | ~32 MB | tracked (compressed, justified); do not grow further |
| `k3iii-bulb5s-capture.log` | ~1.1 MB | tracked; borderline — compress before any further growth |
| other `.log` files | < 200 KB each | tracked, within limits |

The 30-second K-1 II bulb capture was removed from history by an interactive
rebase (see `k1ii-bulb30s-capture.log.gz` for the retained compressed copy);
its SHA-256 is recorded below for provenance of the compressed artifact.

## Checksums

SHA-256 of current evidence files (2026-08-26):

```
205b7facb4b46077bc53b62903a9d8de38711872a584f193d94b698cb0b674e6  firmware-crosscheck-kp-k1ii-k3iii.md
1b790c6aed79a15ab6523e4fc9c0a27cfccb8735cfe6049859a324740d3a93be  k1ii-bulb30s-capture.log (untracked local copy)
05ddd838171d2e1fcc771ef5372834e2e71acf0db3d34d7e046513bf5b5ce398  k1ii-bulb30s-capture.log.gz
bca1930419af4e38bb2ff23d133616ed1ce2ccfa0147d37a89b8ace0404dc16d  k3iii-bulb5s-capture.log
61bdbb285b6ebcc6c8f3d0d83af0e25a02f3d34c932a7eb6333f88613d72bec1  k3iii-bulb5s-retest.log
ec7f29ada984adbc09fb2d21a8c1512de73ac1bc88d1cba6207cd6e1a649f7e4  k3iii-fw-astro-analysis.md
e9ee391bdadb53912f7c9e93c7971c8b0169be7e5e847a6cd66b64cf300da626  k3iii-set-shutter-5s.log
9e45de258b8ef235e5ac303c41b2156c0d401ef300ade3c9e39f2bf79f50073e  k3iii-stale-session-recovery.log
a08991678fd7b0a8a42c33478944dd592aa31892de394e695174d862a5b2b6b6  k3iii-vendor-busy-0x2002.log
```

SHA-256 of the 2026-08-25 evidence set:

```
ff8a67780a8ebab248db4b033e948b2703ca6c3ffed3de1231f81b762c7a7988  k1ii-baseline-live.txt
8c576a8608aa4b59685adcf4714113fe166d407f83d87f24b5ed3ce6a9f165eb  k1ii-bulb-probe-release-mode-0.log
179771a30be68153d033b690e4620d845d501ea920043adb679dd014ff3cbf9c  k1ii-bulb-probe-release-mode-2.log
67f8cd2e834e22dab410114f67896e23ff89d8b72ce2da21b678e28251bb4efc  k3iii-baseline-live.txt
2428f39d711a6ddd66a9e475b9d9da4985ebab753e0d1454445533f13936db7f  k3iii-bulb30s-capture-debug.log.gz
```

SHA-256 of the 2026-09-01 evidence set:

```
54d7265786c8e6e1a766d6b9060c796d06e6fdf93cf0a330f1af4bff67a05305  k1ii-bulb-probe-release-mode-2-rerun.log
5e4a3c1f446f9e658348a9a34d158624aabe529dec46f153907139c9b981a407  k1ii-cardwrite-probe.log
64473629aebca14668f190579e5ef313f9de048d099de05064e1bc1f0a87fe8f  k1ii-cardwrite-probe-green.log
24596beadc09683ecca14eb3f6c9a05af59719be8ffd2acca0f9d6f247a5f69c  k1ii-cardwrite-probe-rerun.log
fc7463461a1be47fd2abb8c0f06b2d2c78fbf41b583a149e7f7596894369ba46  k1ii-cardwrite-probe-restore-off.log
```

Note: the card-write probe logs live in the spec repo at
`docs/pentax/evidence/2026-09-01/` (spec-repo evidence tree); they are
referenced here for provenance. The green-button run
(`k1ii-cardwrite-probe-green.log`) is the first HW confirmation of the
PENTAX_Interrupt 0x9013 opcode: raw PTP RC 0x2001 (PTP_RC_OK). The early
runs (`k1ii-cardwrite-probe*.log`, `--green` run) returned camera-side
PTP_RC_DeviceBusy (0x2019) for all four SetCardWritingMode 0x9004 values;
the later re-run after power-cycle + cable swap
(`k1ii-cardwrite-probe-rerun.log`) returned PTP_RC_OK (0x2001) for all
four modes, and the restore run (`k1ii-cardwrite-probe-restore-off.log`)
verified off(0). The clearing factor (fresh session vs SD card now in SD2)
is not yet isolated.

## Enforcement

A pre-commit style check is recommended upstream; until then, reviewers should
reject Pentax commits that add multi-megabyte uncompressed traces.

SHA-256 of the 2026-09-02 evidence set:

```
3891373e842656407b80628a027e1486aa5fdaf9b75f22ca0be441b886e14a5a  k1ii-crossprocess-readonly-probe.log
c5b75a4921935c86bd78417e640eaf700a5a74b47852bbee219e14f9b6e469eb  k1ii-d02c-probe.log
```

Note: the d02c cross-process probe logs live in the spec repo at
`docs/pentax/evidence/2026-09-02/`. The read-only run shows GET d02c and
GET d02d both returning PTP_RC_DevicePropNotSupported (0x200a) on a K-1 II;
the full SET-sequence run shows SET d02c rejected with
PTP_RC_AccessDenied (0x200f) in both autoselect and cross-process CI modes,
while the d020 precondition/restore writes succeeded and read back. Both RCs
map to GP_ERROR (-1) through `translate_ptp_result` (library.c:88). This
diverges from the session-20 finding on a K-3 III family body, motivating
the k3iii-family-only `pentax_model_supports_cross_process` gate.
