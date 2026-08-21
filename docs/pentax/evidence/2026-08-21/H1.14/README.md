# H1.14 — K-1 Mark II bounded single preview

Camera: PENTAX K-1 Mark II firmware 1.02, PC-P mode, card installed. Serial
omitted.

The current research build retries only `0xa008` (`NoUpdateImage`) at IT2's
33 ms cadence. It checks cancellation before every request and stops at either
30 attempts or 1.5 seconds. Every other response is terminal. Exhaustion is a
timeout and follows the same explicit restoration path. Offline boundary tests,
`ptp2`, the safe harness, and camera-list regression passed before hardware use.

One exact-model frame test returned:

- vendor enable `0x2001`, flags 3;
- the known K-1 II empty PC-LV pre-read (`0x02ff`);
- direct `0xd035=1` start `0x2001`;
- nine transient `0xa008` responses followed by `0x2001` on attempt 10;
- elapsed frame-poll time 312 ms;
- 31,126-byte complete JPEG, validated SOI/EOI in memory;
- no retained file; and
- `gp_camera_exit` cleanup success and released USB ownership.

Status: **SINGLE-FRAME PASS**. This permits the 10-frame paced gate but does not
close stop/restart, reconnect, 50/500-frame, public enablement, zoom, or AF-point
tests.
