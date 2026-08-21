# H1.11 — K-1 Mark II direct property descriptors

Camera: PENTAX K-1 Mark II firmware 1.02 in PC-P mode. Serial omitted.

Nine source-derived properties were queried individually. Every invocation used
an exact model and USB node, a fresh session, a 40-second timeout, one descriptor
read and—only after descriptor success or preservation of an OK-but-unparseable
payload—one value read. No setter, preview, focus, capture, transfer, delete,
reset, shutdown, or firmware operation was used. The entire sequence was
repeated; all descriptor summaries and values were byte-for-byte identical.
The USB node was unowned afterward.

| Code | Parsed descriptor | Stable current payload | Conservative interpretation |
|---:|---|---|---|
| `0xd00f` | UINT64, GetSet, enum 56 | `01000000f4010000` | rational 1/500 s |
| `0x5007` | UINT16, GetSet, enum 22 | `c800` | aperture 200 hundredths, f/2.0 |
| `0xd01e` | UINT32, GetSet, enum 12 | `c8000000` | ISO 200 |
| `0x5010` | INT16, GetSet, enum 31 | `0000` | zero exposure compensation |
| `0x5008` | UINT32, read-only, no form | `88130000` | 5000 hundredths, 50 mm |
| `0x5005` | UINT16, GetSet, enum 18 | `0200` | raw WB value 2; label not correlated |
| `0xd013` | UINT32, GetSet, enum 23 | `01000000` | raw drive value 1; corrects prior UINT8 assumption |
| `0xd02b` | standard parser rejected stable 16-byte payload | `01d00440` | descriptor starts `2bd0044001...` (array-UINT16 type); incompatible with assumed UINT8 scalar; withheld |
| `0xd035` | stable six-byte placeholder `35d000000000` | empty | datatype zero/no value; source-defined PC-LV path only; withheld from descriptor-derived config |

The last two descriptor transactions returned protocol success but their data
did not form ordinary scalar PTP descriptors. Raw preservation was required;
they must not be normalized, guessed, or exposed as writable widgets. Stable
GetSet flags on the other properties are capability evidence, not authorization
to write them.

Status: **READ-ONLY PASS (2/2)** for the seven conventional properties and
**STABLE ANOMALY / WITHHELD** for `0xd02b` and `0xd035`.
