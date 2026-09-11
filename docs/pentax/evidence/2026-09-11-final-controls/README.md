# Final direct-camera controls — 2026-09-11

Source baseline: `90de508a5`, plus the in-review safe-harness robustness fix.
No Polaris firmware was flashed during this test.

| Body | Live view | Focus Near/Far | Shutter round trip |
|---|---|---|---|
| K-1 II `25fb:0183`, `usb:001,047` | PASS, 5/5 valid JPEGs | PASS/PASS, `0x9016`, valid JPEG after each | PASS, `1/100 -> 1/30 -> 1/100` |
| K-3 III `25fb:0189`, `usb:002,009` | PASS, 5/5 valid JPEGs | PASS/PASS, `0x9017`, valid JPEG after each | PASS, `1/5000 -> 1/8000 -> 1/5000` |

PASS for focus means exact-model dispatch, one accepted bounded command,
continued live JPEG delivery, and clean session exit. Physical lens movement
and direction were not operator-observed and remain NOT TESTED.

Raw command output is stored in the sibling `.txt` files. The first K-1 II
Near run exposed a harness-only invalid cleanup value and was overwritten by
the passing rerun after correction; the accepted command and post-command JPEG
were identical in both runs.
