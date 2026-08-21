# H1.19 — source-faithful K-1 Mark II exposure round trips

Camera: PENTAX K-1 Mark II firmware 1.02 in PC-P mode, physical dial `M`.
Serial omitted.

The IT2 source audit showed that live exposure values come from periodic
`GetAllConditions`, while descriptor `CurrentValue` may be stale. IT2 loads
conditions during connection, starts a serialized conditions poll after 100 ms,
writes in a long-lived vendor session, and leaves the session open for later
polls. Earlier tests instead checked the descriptor immediately and exited.

Two exact-model harnesses reproduced IT2's lifecycle: initial conditions,
100 ms, second readiness conditions, one advertised write, up to five 100 ms
live-condition checks, same-session exact restoration on every changed branch,
cleanup, then independent fresh-session verification.

| Property | Round trip | Result |
|---|---|---|
| extended ISO `0xd01e` | 200→400→200 | Both changes appeared in live conditions; both setters returned success; cleanup OK; fresh conditions and descriptor reported 200 |
| shutter `0xd00f` | 1/500→1/125→1/500 | Both changes appeared in live conditions; both setters returned success; cleanup OK; fresh conditions and descriptor reported 1/500 |

Status: **ISO HW-W RESTORED; SHUTTER HW-W RESTORED**. The earlier
acknowledged-but-not-applied classification is superseded: the wire writes were
correct, but immediate descriptor verification was the wrong success oracle.
PTP acknowledgement still remains insufficient without live-condition
verification and exact restoration.
