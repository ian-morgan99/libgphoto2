# Pentax firmware cross-check: KP v1.31, K-1 II v2.51, K-3 III v2.20

Date: 2026-08-26. Files: `fwdc232b.bin` (KP, 18,880,016 B), `fwdc240b.bin` (K-1 II), `fwdc233b.bin` (K-3 III).

## Vendor opcode tables (DeviceInfo blob, ascending u16-LE)

| Firmware | Table offset | Ops |
|---|---|---|
| KP | 0xb4338f | 90 |
| K-1 II | 0xb8062f | 84 |
| K-3 III | 0x1f4f8dc | 88 |

Result: **KP and K-1 II vendor opcode sets are identical**; K-3 III adds only
`0x907a`–`0x907f`. All three share 82 common ops. The IMAGE Transmitter 2
tethering surface is therefore uniform across KP / K-1 II / K-3 III — a driver
path proven on one model should hold on the others.

## Opcodes present in all three firmwares but NOT defined in our ptp.h

`0x9005, 0x9010, 0x9015, 0x901a–0x9020, 0x9023, 0x9024, 0x902d–0x902f,
0x9031, 0x9032, 0x9034–0x9036, 0x9038, 0x903c–0x903f, 0x9041, 0x9042, 0x9044,
0x9047, 0x9049–0x904b, 0x904d–0x9055, 0x9058, 0x9059, 0x905b–0x905e, 0x9060,
0x9062, 0x9063, 0x9067–0x9069, 0x906b, 0x906d–0x9070, 0x9072–0x907f`

These are the live-probe candidate space for astro capture and Star AF.

## Device property tables (d0xx range)

- K-1 II table at `0x9d78e1`, K-3 III first table at `0x1e00c1a`: both begin
  `d050 d0a1 d0a2 … d0fd d1fe` — identical layout.
- KP contains no ascending d0xx prop tables at all (older generation; vendor
  props stored differently). d0xx u16 values appear ~92k times but not as
  DeviceInfo-style arrays. KP likely uses an earlier property scheme; treat
  d0xx probing results from KP as non-transferable.

## Star AF trigger — CORRECTION to earlier finding

The K-1 II firmware **does** contain Star AF code:
- `StarAfReqType:%d` debug printf at file offset `0x21ffd0`
- `StarAfEvent … Unknown flagPattern:%x` at `0x223815`

Both survive only as LZ literals inside compressed spans (no dispatch table
entry identifiable statically). The K-3 III firmware contains neither string —
either absent or fully compressed. Earlier claim "no StarAf strings in either
firmware" was wrong for the K-1 II.

Conclusion: Star AF exists in K-1 II firmware but its vendor opcode remains
unidentified. It is NOT in the shared 82-op set above, so it is either invoked
via one of the undocumented ops, via `0x97ff` GeneralCommand with parameters,
or via a device property write. Live probing required.

## Astro strings

| String | KP | K-1 II | K-3 III |
|---|---|---|---|
| `AstroTr` | absent | 0x342a0e | 0x853b27 |
| `AstroCalcFwhm` | absent | 0x110f1f | 0xa1ab14 |
| `M6AstroT…` | absent | absent | 0x909a92 |
| `Astro.jpg` menu icon | absent | absent | 0x20f3785 |

KP has no astro-tracer tether strings (consistent with no d0xx tables).

## Recommended next probes (all models, non-destructive)

1. Undocumented ops `0x901a–0x9024` with 0/1/2 params — watch for `0x2002`
   (parameter error) vs `0x201e`/OK to fingerprint arity.
2. `0x97ff` GeneralCommand parameter sweeps referencing "ReqType"/"Event"
   naming style.
3. Property writes to d0xx props on K-1 II/K-3 III while in Astro mode.

## Addendum: KF v1.33 (fwdc245b.bin)

Vendor opcode table at `0xb1dadf`, 90 ops — **identical to the KP set**
(KF-only: none; KP-only vs KF: none). No `StarAf`/`AstroTr` strings. The
KP/KF generation shares the same tethering surface and the older (non-d0xx)
property scheme.
