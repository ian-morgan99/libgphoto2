# H1.16 — K-1 Mark II conditions, shutter preflight, and preview gates

Camera: PENTAX K-1 Mark II firmware 1.02 in PC-P mode, physical dial `M`.
Serial and image content omitted. All preview JPEGs were validated in memory and
discarded.

## Shutter-state diagnosis

A fresh read-only `GetAllConditions` response reported exposure mode raw 8
(IT2 `M`), operation state 0, `Tv-changeable=yes`, `task-changing=no`,
`shooting=no`, `processing=no`, ISO 200, and shutter 1/500. This rules out the
obvious IT2 UI gate as the reason H1.15's write was ignored.

The setter was changed to perform the same source-derived conditions preflight
and to reject non-changeable or active states before transmitting. The focused
build and tests passed. One previously authorized 1/500→1/125 attempt then ran:
the preflight passed and the camera again acknowledged the exact payload, but
post-write verification retained 1/500 and the hardened setter returned an
error. A fresh read reported 1/500. An explicit restore-to-1/500 transaction
and independent fresh read both reported 1/500. No preview was conditional on
this failed write. Further shutter writes are closed pending new evidence.

## Preview acceptance sequence

Each gate used a new session, 250 ms inter-frame pacing, cancellation checks,
and retry only for `0xa008`, capped at 30 attempts and 1.5 seconds per frame.

| Gate | Result |
|---|---|
| 10 frames | 10/10 valid JPEGs; frame 1 used 10 attempts/309 ms; frames 2–10 used one attempt; cleanup OK |
| reconnect + 1 frame | Fresh session; valid 27,754-byte JPEG on attempt 8/235 ms; cleanup OK |
| 50 frames | 50/50 valid JPEGs; frame 1 used 9 attempts/280 ms; remaining frames used one attempt; cleanup OK |
| 500 frames | 500/500 successful frame transactions and valid JPEGs; frame 1 used 8 attempts/239 ms; remaining 499 used one attempt; cleanup OK |

The combined stdout/stderr text log split the frame-336 display line during the
500-frame run (`f` before intervening status output and `rame=336...` after it).
The corresponding successful frame transaction is present, the harness counted
500 completed frames, and all 500 frame-response records are PTP OK. This is a
logging interleave artifact, not a missing frame.

Status: **CONDITIONS READ PASS; SHUTTER FAIL-CLOSED/RESTORED; PREVIEW 10,
RECONNECT, 50, AND 500 PASS**. The USB node was unowned after completion.
