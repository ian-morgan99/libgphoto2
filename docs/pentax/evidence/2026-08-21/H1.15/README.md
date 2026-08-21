# H1.15 — K-1 Mark II guarded shutter-write attempt

Camera: PENTAX K-1 Mark II firmware 1.02 in PC-P mode, physical dial `M`.
Serial and image content omitted.

The operator explicitly authorized changing only shutter speed from its current
1/500 s to the descriptor-advertised 1/125 s, followed by a paced ten-frame
preview only if read-back succeeded, and exact restoration to 1/500 s in all
cases.

The initial fresh-session descriptor read reported UINT64, GetSet, current
1/500, and 1/125 as enumeration member 36. `SetDevicePropValue` (`0x1016`) sent
property `0xd00f` with the eight-byte payload `010000007d000000`, exactly the
IMAGE Transmitter 2 representation (UINT32 numerator 1 followed by UINT32
denominator 125). The camera returned PTP OK `0x2001`, but an immediate
same-session descriptor read and an independent fresh-session read both still
reported 1/500. No preview was requested because the prerequisite failed.

An explicit 1/500 restoration write was then sent and an independent read
verified 1/500. The test retained no frame or camera file and the USB node was
released. The result is **ACKNOWLEDGED BUT NOT APPLIED / FAIL-CLOSED**, not
write support. Physical `M` alone is insufficient evidence that IT2's
`IsTvChangeable` condition is true.

Code consequence: the direct shutter path now reads the descriptor on demand
because K-1 II omits `0xd00f` from DeviceInfo, and verifies the descriptor after
every acknowledged write. A retained value is returned as an error instead of
false success. Before another shutter write, perform a read-only condition
audit of the source-derived changeability word at offset 504 and determine the
missing camera state without guessing.
