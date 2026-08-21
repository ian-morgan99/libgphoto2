# H1.4 — Manual-mode read-only configuration baseline

Camera: K-3 Mark III firmware 2.20, physical mode dial `M`. Serial omitted.

## Safety envelope

Three independent candidate sessions enumerated the complete public
configuration tree. Each session performed one vendor enable, descriptor/value
reads, one vendor disable, and clean exit. Sanitized debug logs contained zero
`SetDevicePropValue` (`0x1016`) requests. No shutter, capture, focus, file,
delete, reset, shutdown, or firmware operation was sent.

## Stability result

- each raw public configuration snapshot contained 777 lines;
- after removing serial plus dynamic clock and battery blocks, snapshots 1–3
  were byte-identical;
- each session contained exactly one successful vendor enable and one disable;
- all three sessions returned success.

## Decoded widgets

| Property | Current | Advertised choices | Observation |
|---|---:|---:|---|
| White balance `0x5005` | `0x800f` | 17 | Several existing labels remain unknown; no label inferred |
| F-number `0x5007` | f/3.5 | 19 | f/22 through f/2.8 |
| Exposure compensation `0x5010` | 0 EV | 31 | +5 through -5 EV in third-stop values |
| Shutter `0xd00f` | 1/160 s | 55 | 1/8000 through 30 s |
| Sharpness `0x5015` | midpoint 4 | 9 | Public widget renders 50% |

## Additional raw vendor properties

`Readonly: 0` below reports descriptor GetSet, not proof that a value can be
changed safely in Manual mode. No write has been attempted.

| Property | Current/form summary | Status |
|---|---|---|
| `0xd009` | compound/null public rendering | semantics withheld |
| `0xd013` | current 4; 12 enum values | client-correlated drive-mode candidate; hardware semantics unverified |
| `0xd014` | current 0; singleton enum | client-correlated bracketing mode; unverified |
| `0xd015` | current 10; 15 enum values | client-correlated bracketing step; unverified |
| `0xd01b` | compound/null public rendering | writing-format structure; semantics withheld |
| `0xd01e` | current 3200; 15 values from 100 through 1,600,000 | strong ISO candidate; confirm against camera display before naming/exposing |
| `0xd020` | current 1; 15 enum values | unknown in this hardware record |
| `0xd021`–`0xd023` | current 0; range -4 through +4 | unknown custom-image candidates |
| `0xd024`–`0xd026` | current 0; empty public enum | semantics withheld |
| `0xd027` | current 1; enum 0/1 | semantics withheld |
| `0xd028`–`0xd029` | current 0; range -4 through +4 | semantics withheld |
| `0xd02a` | current 0; enum 0/1 | semantics withheld |
| `0xd02b` | current 0; enum 0/1/2 | semantics withheld |
| `0xd02c` | current 0; empty public enum | semantics withheld |
| `0xd02d` | compound/null public rendering | semantics withheld |
| `0xd035` | current 0; enum 0/1 | USB live view, previously hardware exercised |
| `0xd036`–`0xd037` | compound/null public rendering | live-view controls; semantics withheld |
| `0xd039` | current 0; enum 0/1 | semantics withheld |

Standard ISO `0x500f` and focus mode `0x500a` were absent. Raw descriptors and
camera-display correlation, not numerical resemblance alone, control future
naming. The next comparison baseline is physical Bulb (`B`) mode because long
exposure, bulb timer, and Astrotracer availability may be mode-specific.
