# H1.18 — K-1 Mark II final cold cycle and direct ISO

Camera: PENTAX K-1 Mark II firmware 1.02 in PC-P mode, physical dial `M`.
Serial omitted.

The operator reduced the cold-start repetition target from ten to two after the
10/10 warm gate and extensive fresh-session preview testing. Both cold cycles
confirmed complete USB disappearance and no residual process, followed by a
new USB address, normal-ready operator observation, successful vendor enable
with flags 3, one complete read-only conditions response, clean exit, and USB
release. Status: **COLD 2/2; WARM 10/10** under the revised proportional gate.

The first writable-setting candidate was direct extended ISO `0xd01e`. The
descriptor was UINT32, GetSet, current 200, with advertised choices 100 through
204800; 400 was the nearest non-current choice. A new exact-model widget
requires idle conditions and `Sv-changeable`, transmits only a descriptor value,
and verifies the post-write descriptor.

The existing raw conditions trace has capability word `0x0000000f`, confirming
Av, Tv, Sv, and Xv are all marked changeable. The ISO 200→400 request therefore
passed preflight and was transmitted, but the hardened setter returned failure
because the camera retained 200. An independent fresh session also read 200;
no restoration write was needed. This matches the earlier acknowledged-but-
ignored shutter behavior and suggests a missing K-1 II remote-setting
prerequisite rather than a datatype or individual-property error.

Status: **ISO ACKNOWLEDGED BUT NOT APPLIED / FAIL-CLOSED; ORIGINAL 200 RETAINED**.
Do not try further exposure-property writes until the common prerequisite is
identified. Live-view AF-position and zoom remain separate source-defined
round-trip targets because K-1 II already accepts the PC-live-view property.
