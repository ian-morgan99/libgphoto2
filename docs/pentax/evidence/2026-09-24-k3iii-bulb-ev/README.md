# K-3 III bulb/EV/timer probe — 2026-09-24

## Environment
- PC-side session, no Polaris in the loop.
- K-3 III at `25fb:0189`, `usb:002,002` (firmware 2.20, serial 8093033).
- Driver tree at `c57a8a3ac` (same as the ISO probe session).
- Harness: `examples/pentax-k3iii-bulb-ev-probe.c` (single session, one
  variable per stage), built against `_build/libgphoto2`.

## Stages and results (raw output below)

| Stage | Action | PTP result | Conditions read-back |
|-------|--------|-----------|----------------------|
| baseline | read conditions | — | `bulb-timer=no bulb-seconds=1/20 exposure-comp=0/10 astro-limit=0` |
| bulb | set `pentaxdirectshutter=Bulb` | **GP_ERROR (-1)** | unchanged: `bulb-seconds=1/20` (camera rejected) |
| ev-in-bulb | set `exposurecompensation=-1` | GP_OK (0) | `exposure-comp=0/10` — **not applied** |
| ev-restore | set `exposurecompensation=0` | GP_OK (0) | — |
| 30s | set `pentaxdirectshutter=30s` | GP_OK (0) | `bulb-seconds=30/1` — **applied** |
| 120s | set `pentaxdirectshutter=120s` | **GP_ERROR (-1)** | unchanged: `bulb-seconds=30/1` (camera rejected) |
| restore | set `pentaxdirectshutter=1/20` | GP_OK (0) | `bulb-seconds=1/20` — restored |

## Findings

1. **"Bulb" is not a valid `pentaxdirectshutter` choice string.** The widget
   advertises 55 choices from `1/8000` up to `1s`; whole-second timer values
   (`30s`, `120s`) are accepted by the *setter* (`_put_Ricoh_ShutterSpeed`
   maps `%ds` → `(1<<32)|seconds`) but "Bulb" is not in the advertised
   choice list, so the camera rejects it with a generic PTP error. The
   writable domain for long exposures on this body is the **bulb timer**
   (`bulb-seconds`), not a discrete Bulb shutter value.
2. **Bulb timer boundary: 30s applies, 120s rejected.** `30s` → conditions
   `bulb-seconds=30/1`. `120s` failed with GP_ERROR and the camera kept
   `30/1`. The maximum timer is somewhere in (30, 120] seconds — a 60s
   boundary test is still open (camera was disconnected before it ran).
3. **EV set while shutter=30s-timer: PTP OK but not applied.** Setting
   `exposurecompensation=-1` returned GP_OK, yet conditions still read
   `exposure-comp=0/10`. This is the issue #114 "EV in Bulb" symptom: the
   camera acknowledges the EV write but does not commit it while the bulb
   timer domain is active. (Caveat: the shutter was in 30s-timer mode, not
   a discrete Bulb value, because "Bulb" is not an advertised choice.)
4. **astro-limit stayed 0** throughout; `bulb-timer` flag read `no` even
   after the 30s timer applied (the flag semantics differ from the
   `bulb-seconds` field — the seconds field is the reliable indicator).

## Open items (camera disconnected mid-session)
- [ ] 60s bulb-timer boundary test (harness: `examples/pentax-k3iii-bulb-capture-probe.c`, stage B).
- [ ] ISO 12800 capture + restore (issue #109 acceptance item 5), same harness stage E.
- [ ] Re-test EV-in-Bulb with the camera genuinely in a long-exposure state
      and confirm whether `exposure-comp` ever commits.

## Raw output
See `raw-bulb-ev-probe.txt`.
