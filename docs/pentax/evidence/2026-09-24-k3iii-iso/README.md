# 2026-09-24 — K-3 III ISO capability probe (issue #109 / libgphoto2#77 downstream)

## Environment
- Host: ian@beast, PC-side (no Polaris in the loop).
- Camera: PENTAX K-3 Mark III, `25fb:0189`, `usb:002,002`, firmware 2.20, serial 8093033.
- Driver: patched libgphoto2 worktree `~/Documents/VSCodeProjects/LibGphoto2/libgphoto2`
  at `c57a8a3ac` (master, "preserve capture cancellation in the post-capture idle wait (#122)").
  Pre-existing WIP edits (k01/readiness) were stashed as
  "agent: WIP k01/readiness edits (pre K-3 III PC ISO test)" before rebuilding;
  `ptp2.so` relinked clean.
- Run env: `LD_LIBRARY_PATH=_build/libgphoto2:_build/libgphoto2_port/libgphoto2_port`,
  `CAMLIBS=_build/camlibs`, `IOLIBS=_build/libgphoto2_port/libusb1`.
- Harness: `examples/pentax-k3iii-iso-probe.c` (new, untracked), built to /tmp/k3iii-iso-probe.

## Sequence (single session, one variable)
1. baseline read via `pentaxconditions`: ISO=100
2. advertised `iso` widget choices (type 5 = RADIO, 15 choices):
   100 200 400 800 1600 3200 6400 12800 25600 51200 102400 204800 409600 819200 1600000
3. SET 12800: set_result=0, conditions read-back ISO=12800 (applied)
4. SET 99999 (invalid): set_result=-1 (clean failure), ISO unchanged at 12800
5. restore 100: set_result=0, conditions read-back ISO=100 == baseline

## Result
PASS — the patched driver advertises and applies >6400 ISO on K-3 III
(12800 verified via conditions read-back), invalid values fail cleanly,
and the original value restores. The 6400 truncation in issue #109 is
therefore NOT in this libgphoto2 build's `iso` widget; it must live in the
pgphoto/Stage-2/Benro protocol layer or the client UI (per #109 task list).

## Raw output
```
stage=baseline conditions_iso=100
stage=choices type=5 count=15 value=100: 100 200 400 800 1600 3200 6400 12800 25600 51200 102400 204800 409600 819200 1600000
stage=highiso set_result=0 conditions_iso=12800
stage=invalid set_result=-1 conditions_iso=12800
stage=restore set_result=0 conditions_iso=100 expected=100
iso_probe=pass cleanup=ok
```

## Notes / caveats
- Model string for exact selection: `Pentax K-3 Mark III (MTP mode)` (no colon;
  `gp_abilities_list_lookup_model` matches the table name verbatim).
- No capture was fired in this probe (ISO set/read-back only); a >6400
  capture+restore is still open per #109 acceptance item 5.
- WIP stash to restore after this session: `git stash list` →
  "agent: WIP k01/readiness edits (pre K-3 III PC ISO test)".
