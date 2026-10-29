# IMAGE Transmitter 2 capability target matrix

Revision: 2026-10-29 (content largely from 2026-08-21; the `0x0182` amendment
below is from 2026-10-06, commit `f3a8ffebf`; the format-domain correction and
model-gate audit are from 2026-10-28; the IT2 2625 Monochrome correction is
from 2026-10-29).

This is the normative target for Pentax development in this fork. It
consolidates the extraction previously split across `PENTAX_WIRE_PROTOCOL.md`,
`PENTAX_CONFIGURATION.md`, source notes, and hardware evidence. DeviceInfo
omission means “not advertised”, not “unsupported”: IMAGE Transmitter 2 (IT2)
directly requests many omitted vendor codes.

The complete IT2 UI-to-wire setting audit and its remaining session/WPD
differential are recorded in `IMAGE_TRANSMITTER_SETTING_PATH.md`.

Status terms: **IT2** means directly present in the IT2 client as observed; **HW-R**
means read-only hardware-confirmed; **HW-W** means bounded write plus exact
restoration confirmed; **target** means not yet closed; **withheld** means not
available through ordinary libgphoto2 APIs. Source anchors are member names in
`MtpDevice`, `MtpOpCode`, `ConditionIndex`, `ModeEnableInfo`, `ExpMode`, and
`DriveModeLUT`; line numbers are intentionally avoided.

## Static audit and test coverage

The 2026-10-29 audit is host-side and does not replace a camera test. Its
regression contracts are deliberately split by layer:

| Contract | Evidence and test |
|---|---|
| Model identity and IT2 capability flags | IT2 2625 `MtpDevice.Model` gate, recorded in `REFERENCE_CLIENTS.md`; `tests/test-pentax-utils.c` checks K-1 II 78400, K-1 77970, K-3 III 78420, and Monochrome 78550 plus every audited flag |
| Modern format enums and payloads | `ptp.h` keeps `ShootingFormat`, `WritingFormatSetting`, `RawKind`, and `TransFileFormat` separate; `test-pentax-utils` checks their values, TIFF rejection by the modern widget contract, 10/11-byte validation, and byte-preserving format/raw-kind changes |
| K-5 II format contract | `tests/test-pentax-legacy-parsers.c` checks the legacy USB-SCSI model identity and its independent JPEG/RAW/RAW+JPEG and PEF/DNG enum domains; no K-5 II row is added to the modern PTP model table |
| Focus and AF | `test-pentax-utils` checks old `0x9016` versus new `0x9017` dispatch and direction semantics; `test-pentax-aperture-alias` checks the generic policy/manual-focus/immediate-AF widget contract for old and new families |
| StarAF | The public control test asserts that no speculative `staraf`, `starafmode`, `starafdrive`, or `afwithliveview` widget is registered. IT2's AF-when-released/TouchAF policy remains distinct from an identified StarAF command |
| Colour-only control boundary | The public fixture seeds a valid `0xd018` descriptor and proves the colour K-3 III exposes `pentaxcolortemperature` while model 78550 Monochrome rejects it |

## Model gates

The four modern bodies in this audit have separate rows even where their
operation codes are shared.  `Yes` below means that IT2's static model flags
enable the corresponding client path; it is not a hardware qualification.

| Gate | K-1 Mark II | K-1 | K-3 Mark III | K-3 Mark III Monochrome |
|---|---:|---:|---:|---:|
| Exact model / USB | PC-P `PENTAX K-1 Mark II`, `25fb:0183`; MSC `25fb:0182` | `PENTAX K-1`, `25fb:0179` | `PENTAX K-3 Mark III`, `25fb:0189` | `PENTAX K-3 Mark III Monochrome`, `25fb:018f` |
| Vendor model / extension | 78400 / 1 | 77970 / 1 | 78420 / 1 | 78550 / 1 |
| Dual slot / new transfer / movie | Yes / Yes / Yes | Yes / Yes / Yes | Yes / Yes / Yes | Yes / Yes / Yes |
| New focus control | No: old `0x9016` | No: old `0x9016` | Yes: new `0x9017` | Yes: new `0x9017` |
| Movie settings / movie sub-image | No / No | No / No | Yes / Yes | Yes / Yes |
| Tv / Av / exposure bracket gates | No / No / No | No / No / No | Yes / Yes / Yes | Yes / Yes / Yes |
| Composition adjustment / aperture keep | No / No | No / No | Yes / Yes | Yes / Yes |
| Color-temperature UI / detailed CI | No / No | No / No | Yes / Yes | **No / Yes** |
| New timer/remote continuous modes | No | No | Yes | Yes |
| Current-writing-slot / high-resolution PC LV | No / No | No / No | Yes / Yes | Yes / Yes |
| Monochrome model flag | No | No | No | Yes |
| Satobi | firmware >=1.0 | firmware >=1.0 | firmware >=1.3 | not enabled by the model gate |

The tested firmware versions, K-1 II 1.02 and K-3 III 2.20, satisfy Satobi.
Runtime `0x9001` function flags are separate: K-1 II returned 3; K-3 III 0.

The K-1 II USB identities are separate personalities. `25fb:0183` exposes an
Imaging/PTP interface and is the only one eligible for the Pentax `ptp2`
control path. `25fb:0182` exposes only USB Mass Storage class `08/06/50`
(SCSI transparent, Bulk-Only); it must use the operating-system mounted-volume
or libgphoto2 Directory Browse path. MSC supports file enumeration, copying,
hashing, and metadata reads, but not live view, capture, configuration, or
focus control.

**Amended 2026-10-06 (#179).** This section previously said *"never add `0182`
to the `ptp2` ability table"*. That instruction is superseded: `0x0182` is now
present as `Pentax:K-1 Mark II (MSC mode, USB id 0182)` with an **empty ability
set**. The reason is diagnostic, not functional. Every Pentax body declares a
PID pair in which the first ID is MSC and the second is PTP, so a body
presenting `0x0182` is in MSC mode; before the row existed such an attachment
autodetected to *nothing* and the host silently applied hardcoded K-3 III
abilities. With the row, the failure is a PTP error against the correct model.
The rule that survives, and the one that actually matters, is: **`0x0182` must
never carry Pentax control abilities.** Adding it to the table with a non-empty
ability set would be the original error. The remedy for an MSC-mode body is the
camera's USB mode setting, not a driver entry. See
`REAL_HARDWARE_TEST_LOG.md` and commit `f3a8ffebf`.

## Model coverage status — what "supported" means per body

Updated 2026-10-29. Asked directly: *does this work on all Pentax cameras?* The
honest answer is **no, and the gap is measurable.** `camlibs/ptp2/pentax-utils.h`
declares 11 modern IT2 model IDs and `pentax-utils.c` recognises those IDs from
USB strings, but only two bodies have any hardware evidence. The other bodies
are recognised so
that they fail *correctly*, not so that they work.

| Model | ID | Recognised | Code references¹ | Hardware evidence |
|---|---:|:--:|:--:|---|
| K-3 Mark III | 78420 | yes | 6 | **HW-R / HW-W** — primary development body; direct-PC B-mode held Bulb action PASS for four 1 s starts/stops on PTP PID `0x0189`, with saved DNG/JPEG and EXIF 1.064–1.073 s. This qualifies only the experimental held action, not timed natural completion or Polaris. |
| K-1 Mark II | 78400 | yes | 3 | **HW-R / HW-W** — second qualified body |
| K-3 Mark III Monochrome | 78550 | yes | 4 | no held-Bulb hardware evidence; PTP PID `0x018f` is explicitly blocked from the experimental action; IT2 2625 has a specific Monochrome gate, with colour-temperature support disabled |
| KP | 78380 | yes | 3 | none. Classified **new-focus** (`0x9017`) by IT2's model table, unverified on hardware |
| GR III | 78350 | yes | 2 | none. Also classified new-focus, unverified |
| 645Z | 77840 | yes | 3 | none |
| K-1 | 77970 | yes | 2 | none |
| K-3 | 77760 | yes | 2 | none |
| 645D | 77320 | yes | 2 | none |
| K-70 | 78370 | yes | 1 | none |
| KF | 78520 | yes | 1 | none |
| K-5 II | — | legacy only | — | Not an IT2 model. Remains in the USB-SCSI/MSC `camlibs/pentax` path; no modern `ptp2` vendor controls are inferred. |

¹ Count of `PENTAX_MODEL_*` occurrences in `pentax-utils.c` + `library.c`,
including the recognition assignment itself. It measures how much code branches
on the model, not how much of it is verified.

Read this as: **2 of 11 modern IT2 bodies have general hardware evidence.** Everything
else is recognition-plus-fail-closed. Most capability predicates remain derived
from IT2's model table and encode what the vendor client does, not what a body
has been observed to do. The exception is the experimental held-Bulb product
gate: standard K-3 III PID `0x0189` is enabled from the direct-PC result above;
Monochrome PID `0x018f` remains blocked. That one-second held-action pass does
not qualify the camera-timed or Polaris path.

Two consequences that matter for planning:

1. **The new-focus classification is the highest-risk unverified claim.** It is
   inferred from IT2, and it decides whether `0x9016` or `0x9017` is sent. If KP
   or GR III is actually old-focus, focus control sends an opcode the body does
   not implement. One bounded read-only probe per body settles it.
2. **Do not advertise support for any body without a row in
   `REAL_HARDWARE_TEST_LOG.md`.** Recognition in a table is not support, and
   "it enumerated" is not support. The K-1 II and K-3 III already proved that
   superficially similar Pentax bodies differ in transfer path, focus family and
   capability flags.

The per-model sweep required by #52 (`MODEL_SWEEP_PUBLIC_SUMMARY.md`) is the
correct place to close this; this table is the current honest state, not a
substitute for that sweep.

## Vendor operation matrix

| Code | IT2 meaning and transport | K-1 II / K-3 III | Fork and hardware status |
|---:|---|---|---|
| `0x9001` | vendor mode; no data; `[13,model,on/off,1,0]`; Param2 flags | Both | Implemented; HW-R both |
| `0x9002` | camera shutdown; no parameters | Both | Withheld; never a wake/recovery command |
| `0x9003` | acknowledge old-path object handle | Legacy path | Not the target for these new-transfer bodies |
| `0x9004` | card writing bitmask | Both | Target; no hardware write |
| `0x9006` | live-view frame, data-in | Both | Implemented; K-3 III 50-frame HW-R; K-1 II 10/reconnect/50/500-frame HW-R, with bounded transient startup polling and cleanup |
| `0x9007` | old-path sub-image, handle, data-in | Legacy path | Not target for these bodies |
| `0x9008` | old-path main image, handle, data-in | Legacy path | Not target for these bodies |
| `0x9009` | warning-message flag | Both | Target/low priority |
| `0x900a` | InvokeCameraTask name only; no active call found | Unknown | Withhold pending call-site/trace |
| `0x900b` | candidate metadata; type 0 or preview 253, data-in | Both | Research implemented; hardware target |
| `0x900c` | file command: operation byte + signed count/seek | Both | Research implemented; hardware target |
| `0x900d` | transfer block; requested count; Param1 valid count | Both | Research implemented; 8 MiB safety cap |
| `0x900e` | finalize/delete candidate, data-in | Both | Research implemented; only after host cache |
| `0x900f` | all conditions, data-in, no parameters | Both | Read-only implemented; 568-byte HW-R both |
| `0x9011` | initiate: release, focus, MWB, sync, aperture reset | Both | Research-only held action; direct-PC B-mode PASS on K-3 III PTP PID `0x0189` for four 1 s captures; production remains withheld |
| `0x9012` | terminate with release mode | Both | Research-only matching stop; same bounded direct-PC PASS; natural-completion race and Polaris integration NOT TESTED; PID `0x018f` blocked |
| `0x9013` | interrupt/Green button | Both | Target; not generic guessed cancellation |
| `0x9014` | SetLightMeteringMode name only; no active call found | Unknown | Withhold pending call-site/trace |
| `0x9016` | old focus: amount, direction | K-1 II | Hardware target; no K-1 II focus command yet |
| `0x9017` | new focus: signed displacement | K-3 III | Bounded Near/Far HW-W |
| `0x9018` | composition direction/steps; two response values | K-3 III gate | Target |
| `0x9019` | aperture keep flag/value | K-3 III gate | Target |
| `0x9021` | WB measurement name; no active call found | Unknown | Withhold pending call-site/trace |
| `0x9022` | dust-reduction name; no active call found | Unknown | Withhold pending call-site/trace |
| Star AF trigger | K-1 II fw contains `StarAfReqType:%d` / `StarAfEvent` debug strings (LZ literals @0x21ffd0/0x223815 in fwdc240b), but no IMAGE Transmitter 2 call path or opcode is identified; the K-3 III static scan likewise identifies no trigger | Firmware evidence only for K-1 II | Not implemented. The IT2 **AF when released** policy is separate and does not establish Star AF support. The former `0xd038 = AF Active Area` candidate is retired by issue #76; see evidence/2026-08-26/firmware-crosscheck-kp-k1ii-k3iii.md |
| Astro-mode capture | ExpMode 20 rejects vendor 0x9011 with 0x2002 on the wire; astro capture path not in any identified opcode — candidates are undocumented 0x901a-0x9024 or standard 0x100E | K-3 III | Open; live probe of undocumented ops pending camera availability |
| `0x97ff` | general command plus four parameters; preview uses command 1 | Both | Withhold pending trace |

Generic storage enumeration/download is a separate path and does not prove the
vendor capture/candidate workflow.

## Direct property matrix

| Code | IT2 logical control | Wire representation and gate | Current target status |
|---:|---|---|---|
| `0x5001` | battery level | UINT8 on LS path; K bodies primarily use conditions | Generic/aggregate status target |
| `0x5005` | white balance | UINT16; source LUT; both | K-3 III HW-R; K-1 II direct descriptor target |
| `0x5007` | aperture | UINT16 hundredths; both | K-3 III HW-R; K-1 II target |
| `0x5008` | focal length | UINT32 hundredths; status on K bodies | K-1 II lens correlation target |
| `0x500a` | LS focus mode | UINT16; LS only | Never substitute for K-body AF mode |
| `0x500f` | legacy ISO | UINT16 fallback | K bodies use `0xd01e` |
| `0x5010` | exposure compensation | INT16 thousandths; both | K-3 III HW-R; K-1 II target |
| `0x5014` | CI contrast | UINT8, logical value +4 | Source target |
| `0x5015` | CI sharpness | UINT8, logical value +4 | K-3 III HW-R; writes pending |
| `0xd009` | LV/CAF geometry | 20 bytes with UINT16 dimensions | Fail-closed parser; K-1 II HW-R 720×480, K-3 III target |
| `0xd00f` | shutter/Bulb timer | UINT64 as UINT32 numerator/denominator | K-3 III HW-R; K-1 II source-faithful 1/500→1/125→1/500 HW-W restored; verify through live conditions, not descriptor alone |
| `0xd013` | drive mode | K-1 II descriptor is UINT32 enumeration; do not assume one cross-model width | K-1 II HW-R enum 23/current raw 1; K-3 III width re-audit target |
| `0xd014` | bracket mode | UINT8; K-3 III gate | K-1 II withheld |
| `0xd015` | bracket step | UINT8 tenths; K-3 III gate | K-1 II withheld |
| `0xd018` | WB color temperature | UINT16, `FormFlag == None` (no form, no value list); K-3 III UI gate | HW-R + HW-W on the K-3 III through the raw property path (read 5000, wrote 4000, restored 5000). The named `pentaxcolortemperature` control was withheld until 2026-10-19: `_pentax_prop_get` rejected every `FormFlag == None` descriptor, conflating this writable scalar with the empty-enumeration case. Fixed in #104 by admitting only a `GetSet` scalar with no form; `Enumeration` with `NumberOfValues == 0` still fails closed. No range is invented: the descriptor supplies none, so the control renders as a plain integer/TEXT widget, matching this row's descriptor-driven intent. Named-control hardware re-qualification is the remaining gate. |
| `0xd01b` | file format/size/quality/RAW/card/movie | compound 10/11-byte structure | Source-authorised K-1/K-1 II/K-3 III/Monochrome path; exact-length byte-preserving read-modify-write and raw read-back are implemented; independent restoration and transfer remain hardware gates |
| `0xd01e` | extended ISO | UINT32 enumeration | K-3 III HW-W restored; K-1 II source-faithful 200→400→200 HW-W restored; verify through live conditions, not descriptor alone |
| `0xd020` | CI mode | UINT8 | Base-CI source target |
| `0xd021` | CI saturation | signed byte | Descriptor-driven implemented; hardware target |
| `0xd022` | CI hue | signed byte | Descriptor-driven implemented; hardware target |
| `0xd023` | CI high/low key | signed byte | Descriptor-driven implemented; hardware target |
| `0xd024` | CI filter | UINT8; UI 9 maps to wire 255 | Descriptor-driven implemented; hardware target (confirm descriptor enumerates 255) |
| `0xd025` | CI monotone toning | signed byte | Descriptor-driven implemented; hardware target |
| `0xd026` | CI toning | UINT8 | Descriptor-driven implemented; hardware target |
| `0xd027` | CI fine sharpness | UINT8 | Descriptor-driven implemented; hardware target |
| `0xd028` | CI highlight contrast | signed byte; detailed-CI gate | Descriptor-driven implemented (K-3 III gate); hardware target |
| `0xd029` | CI shadow contrast | signed byte; detailed-CI gate | Descriptor-driven implemented (K-3 III gate); hardware target |
| `0xd02a` | composition-adjustment switch | UINT8 boolean; composition gate | K-3 III target |
| `0xd02b` | focus peaking | IT2 assumes UINT8; K-1 II instead returns stable `0x4004` array-UINT16 descriptor payload and anomalous 4-byte value | K-1 II withheld; K-3 III hardware target |
| `0xd02c` | CI cross process | encoded UINT8 normal and 32+ ranges | Source target |
| `0xd02d` | CI user filter | 8-byte signed triplet structure | Read-only descriptor passthrough implemented; write needs array codec |
| `0xd035` | PC live view | IT2 writes UINT8 boolean for both K-1 and K-1 II; the K-1 II descriptor is a datatype-zero six-byte placeholder with empty value, so IT2 starts directly without a preliminary GET | K-3 III HW-W restore; K-1/K-1 II direct-start path now follows IT2, with one-frame restore target; K-1 hardware qualification remains pending |
| `0xd036` | LV AF position | 4-byte response means geometry centre; >=8 bytes has X/Y UINT16 at 4/6; setter is `{2,0,0,0,Xlo,Xhi,Ylo,Yhi}` | Parser/encoder fixtures; K-1 II HW-R 8-byte centre 360,240; bounded write target |
| `0xd037` | LV zoom | `{4,0,0,0,Xlo,Xhi,Ylo,Yhi,mag,0,0,0}`; disable mag 1 | Encoder fixture; K-1 II HW-R raw 1; public config target; 16x fallback once to 10x only on `0x201c` after hardware gate |
| `0xd039` | movie mode | UINT8 boolean; movie gate | Both source target |

`0xd01b` offsets are: 0 count/version (6 normally, 7 with current-slot field),
4 and 5 the IT2 `WritingFormatSetting` (`0=JPEG`, `1=RAW`, `2=RAW+JPEG`,
`3=TIFF`), 6 still/movie size, 7 JPEG quality (`2-UI`), 8 RAW kind or movie
frame rate (UI 2 maps to 3), 9 one-based card slot, and 10 one-based current
slot for K-3 III. `ShootingFormat` is a different status enum used for the
actual output reported for SD1/SD2 at conditions offsets 464/468
(`0=JPEG`, `1=PEF`, `2=DNG`, `3=PEF+JPEG`, `4=DNG+JPEG`, `5=TIFF`).
`TransFileFormat` is a third enum used as the type argument to `0x900b`
(`0=Capture`, `1=JPG`, `2=PEF`, `3=DNG`, `4=TIFF`, `253=Full`, `254=Sub`,
`255=Thumb`). It requires a fresh, byte-preserving read-modify-write; no
mutation is allowed until each body’s returned form is captured and
fixture-tested. The generic `imageformat` widget for the currently enabled
K-1/K-1 II/K-3 III/Monochrome path deliberately exposes only `JPEG`, `RAW`,
and `RAW+JPEG`; PEF/DNG selection belongs to the separate IT2 `RawKind`
control and is now exposed as `pentaxrawformat` (`PEF`/`DNG`). A generic
`imageformat` write changes only 0xd01b bytes 4/5 and preserves byte 8, so a
client reading `RAW` and writing `RAW` cannot silently convert PEF to DNG.
The raw-kind write changes only byte 8 and uses the same exact-payload
read-back gate. TIFF is source-defined for the 645Z UI and is not accepted by
the current modern-family gate.

## Complete condition-layout target

| Offset(s) | IT2 meaning | Fork status |
|---:|---|---|
| 12, 16, 24 | DSP flags; cancel bits; operation state | operation/Astro subset implemented |
| 32, 36 | candidate flag and handle | Research implemented |
| 40 | user mode | Raw implemented |
| 80, 84/88, 92 | bracket enabled, step, frame code | Target |
| 104, 108 | activity bitfield; ISO auto setting | activity subset implemented |
| 120 | white balance | Target |
| 132, 136, 140, 144, 148 | format, size/rate, quality, RAW, CI | Target |
| 168, 172 | exposure and sensitivity steps | exposure step implemented |
| 184, 188, 196 | exposure, program, AF modes | exposure raw implemented |
| 216, 220, 224, 232 | legacy drive; hyper flags; roll; bracket count | Target |
| 252 | color temperature | Target |
| 272/276, 280/284, 288/292 | shutter, aperture, exposure-comp rationals | All parsed; K-1 II HW-R 1/500, 20/10 (f/2.0), and 0/10 EV |
| 304, 312, 316, 320, 328 | max Tv, ISO, program, status flags, open Av | ISO/status/open Av implemented |
| 368, 420 | battery and focal length | Target |
| 436–452 | flash state/modes/type | Target/low priority |
| 456/460, 464/468, 472 | card remaining, formats, temperature | Target |
| 476/480, 484/488, 492 | card states/activity, new drive | drive raw implemented |
| 496, 500, 504 | LV AF result, rotation, changeability/Bulb/GPS/Astro | 504 subset implemented |
| 512/516 | crop/aspect | Target |
| 520, 524, 528 | card-slot, writing format, Astro limit | Astro limit implemented |
| 532–556 | composition state/axes/rotation/limits | K-3 III target |
| 560, 564, 568 | movie card slot, electronic shutter, current slot | Model-gated targets |

Minimum safe lengths are field-specific. The current public subset requires
508 bytes and reads Astro limit only at 532 bytes or more. The 568-byte K-1 II
response must not be read at offset 568; IT2 checks for length greater than 568.

`ExpMode` maps P 0, Auto Picture 1, Hyper-P 2, Green 3, Tv 4, Av 5, Hyper-Tv 6,
Hyper-Av 7, M 8, B 9, aperture-ring Av/M/B 10/11/12, TAv 13, ring-TAv 14, Sv
15, X/ring-X 16/17, lens-shutter 18, APL-P 19, Astro 20, Scene Auto 21,
advanced Hyper modes 22–30, and Hyper-Sv 31. K-1 II B is HW-R as 9. The manual
photo makes 21 only a provisional Scene Auto correlation until confirmed in a
same-setting session.

The 52-entry drive LUT is source evidence for internal/UI ordering, but labels
come from UI image order rather than a protocol enum declaration. Preserve raw
values until resource ordering and camera state jointly confirm each label.

## Mandatory target order

| Tier | Work | Exit gate per applicable body |
|---:|---|---|
| 0 | identity and public containment | exact USB/model; capture/preview/trigger default-off |
| 1 | source matrix | every row traced to source member/call site |
| 2 | lifecycle | 10 cold + 10 warm; fail-closed; exact cleanup |
| 3 | direct descriptors | individually read every applicable code; record response/type/form |
| 4 | aggregate status | three snapshots per representative mode; visible correlation |
| 5 | generic files | repeat RAW hash and one JPEG hash without card deletion |
| 6 | scalar writes | one at a time; read-back, display, exact restore |
| 7 | live view | Expose through `gp_camera_capture_preview`; 1/10/50/500 frames; bounded consecutive-frame failure; restore zoom/AF/PC-LV, reconnect, cold reconnect |
| 8 | live-view controls and focus | Config widgets for zoom and AF position, action/range widgets for fine focus; K-1 II one bounded `0x9016` Near then equal Far; K-3 III repeat `0x9017`; zero automatic displacement escalation |
| 9 | JPEG capture/transfer | research build; cache, finalize, reconnect |
| 10 | RAW/RAW+JPEG | hash, finalize, repeat, reconnect |
| 11 | Bulb/cancel/recovery | short supervised B exposure, terminate, reconnect |
| 12 | advanced features | only model-gated and source-complete rows |
| 13 | Polaris | exact fork packaged; applicable rows pass through UI |

Rules: source support, hardware support, implementation, and public enablement
remain separate. DeviceInfo cannot close IT2 rows. K-3 III flags never flow to
K-1 II through shared methods. Descriptor reads precede values and values
precede writes. Compound writes require length and preservation fixtures.
