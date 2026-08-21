# H1.13 — K-1 Mark II instrumented preview localization

Camera: PENTAX K-1 Mark II firmware 1.02, PC-P mode, card installed. Serial
omitted.

The current research build added exact-stage reporting and an exact-K-1-II
fallback for its already-confirmed empty/datatype-zero `0xd035` value. In a
fresh non-live-view session only, that fallback records the original state as
stopped, follows IT2's direct start/stop path, and still requires explicit stop
restoration. Offline `ptp2`, safe harness, parser, and camera-list builds/tests
passed before hardware access.

One exact-model, one-frame invocation returned:

- vendor enable: `0x2001`, function flags 3;
- PC-LV pre-read: `0x02ff` (empty value cannot be decoded as UINT8);
- `0xd035=1` start: `0x2001`;
- first `0x9006` frame request: `0xa008`, zero bytes;
- `0xd035=0` restoration after frame failure: `0x2001`; and
- USB released after exit.

IT2 names `0xa008` `NoUpdateImage`. Its 33 ms timer simply emits no frame event
for a non-OK response and polls again later. This proves the earlier composite
`GP_ERROR_IO` was a too-early/no-frame-yet response, not failure to start or
restore live view.

Status: **START/RESTORE PASS; FRAME NOT READY**. Implement a bounded polling
policy specifically for `0xa008`, with cancellation, elapsed-time and attempt
limits. Every other response remains terminal. Test it offline before one more
single-frame attempt.
