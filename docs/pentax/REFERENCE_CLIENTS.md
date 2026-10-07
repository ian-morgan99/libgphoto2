# Reference clients: what the working Pentax apps actually do

Revision: 2026-10-07.

Ricoh ships tethering software that works on hardware where our support is
partial. Those clients are the closest thing we have to a specification of
vendor-mode behaviour, so claims about "how the camera must be driven" should be
traced to one of them rather than inferred from our own failures.

This file is the index of what is available, what each source can and cannot
prove, and where its copies live. It exists because the same question has been
re-derived several times without anyone recording which artifact was read.

## The three reference clients

| Client | Transport | Location | State | Can prove |
|---|---|---|---|---|
| IMAGE Transmitter 2 (Windows/.NET) | **USB PTP** via Windows WPD | `/home/ian/Documents/VSCodeProjects/LibGphoto2/ImageTransmitter2/` | Decompiled source, readable | Vendor opcode semantics, condition offsets, session ordering, timing |
| Wi-Fi Commander Pentax v1.76 (Windows/.NET) | **WebSocket to the camera's own Wi-Fi server** (`192.168.0.1`) | `/home/ian/Documents/VSCodeProjects/LibGphoto2/wificommanderpentax_v176/` | Installer + extracted `wificommander.exe`; **app assembly NOT decompiled** | That a feature exists at all; command *names*. Not PTP semantics |
| Pentax Image Sync (Android) | Camera Wi-Fi server (same family as above) | not held locally | absent | Same as Wi-Fi Commander, if obtained |

**Only IMAGE Transmitter 2 is a PTP reference.** Wi-Fi Commander and Image Sync
talk to the camera's embedded Wi-Fi service over a JSON-ish command channel
(`camera_shoot`, `camera_shoot_bulb`, `SMinterval*`, …). They never issue a PTP
operation. They are evidence about *product behaviour and feature intent* — what
a Bulb control means to a user, what a client is expected to poll — and they are
**not** evidence about wire-level PTP. Do not cite them for opcode or offset
claims; that mistake makes a Wi-Fi protocol fact look like a PTP fact.

## IMAGE Transmitter 2 — provenance of the copy we read

The decompiled tree is not under version control. Pin it by content hash before
citing it:

```text
5c6b0b68adb5ef79235a1c40506f0b53  IMAGETransmitter2/MtpDevice.cs      (6961 lines)
039b23e6ee8cce8317556e6855997758  IMAGETransmitter2/MainWindow.xaml.cs
8d440f053038dcdedcf1d6e7ae9774ab  IMAGETransmitter2/ConditionIndex.cs
```

Key files: `MtpDevice.cs` (all PTP traffic and the `GetAllConditions` decoder),
`ConditionIndex.cs` (the field index → name table), `ExpMode.cs` (exposure-mode
IDs), `MainWindow.xaml.cs` (UI-driven timing: release, Bulb timer, live view).

Citations in `PENTAX_WIRE_PROTOCOL.md`, `IMAGE_TRANSMITTER_*.md` and
`K3III-CAPTURE-MODE-DIAGNOSTICS.md` are against this tree. Line numbers drift
between decompiler versions — cite the symbol name and the hash, not the line.

## Verified behaviours worth treating as settled

Re-checked against the source on 2026-10-07. Each of these contradicts or
sharpens something previously assumed:

1. **Condition polling is a self-rearming one-shot, not a 100 ms period.**
   `ConditionRefreshTask` disarms the timer, issues `0x900f`, performs any
   candidate download, then calls `Change(100, -1)`. The 100 ms is an idle gap
   measured from completion of the previous cycle. Two early `return` paths skip
   the rearm and stop polling silently. `PENTAX_WIRE_PROTOCOL.md` said "polls
   every 100 ms"; corrected 2026-10-07.
2. **All MTP traffic is serialized under one lock** (`_wpdCmdLock`). Timer
   polls and user actions never overlap on the bus. A client that interleaves
   polls with downloads is not behaving like the reference.
3. **Bulb is two mechanisms and only one is ours to control.** The camera's own
   Bulb timer is state read from conditions (offsets 272/276 numerator/
   denominator, capability bit `1<<6` at offset 504). The *duration* a user
   types is application state (`TSBulbTime`) and is implemented as a second
   `CamRelease()` edge after N one-second ticks — not as a shutter-speed write.
   See `K3III-CAPTURE-MODE-DIAGNOSTICS.md` #113.
4. **Bulb is an exposure mode, not a shutter value.** `ExpMode.B = 9`; the
   client treats modes 9, 12 and 20 as the Bulb family and *hides* the shutter
   control in all three. A client that sends a shutter string for Bulb is
   inventing a mechanism the camera does not have.
5. **The condition offset table in `PENTAX_WIRE_PROTOCOL.md` is correct.**
   Independently re-derived from the decoder: 312 = ISO, 272/276 = Bulb/shutter
   fraction, 288/292 = exposure compensation, 504 = capability bits, 492 = drive
   mode, 184 = exposure mode, 104 = activity bits. IT2 additionally decodes
   offsets we do not parse (12, 16, 84, 88, 108, 120, 148, 216, 220, 252, 368,
   420, 448–500, 520–568); absence from our table means unparsed, not unused.
6. **IT2 does not contain raw PTP opcode literals.** It calls WPD, which uses
   device-io-control GUIDs. Opcode names in our docs come from the property/
   operation tables in `ptp2`, cross-checked against IT2's *behaviour*. Do not
   claim an opcode is "used by IT2" from a string search of its source.

## Wi-Fi Commander — what is actually available

`_decompiled/` contains 338 third-party files (log4net, SuperSocket,
WebSocket4Net, SharpZipLib). The application assembly itself decompiled to a
**zero-byte file** and its protocol code has never been read. The installer and
extracted binary are present and hash-stable:

```text
2494b4cb862d8cae512cefde6d980ab5  _extracted/cab/wificommander.exe (1 569 280 B)
c575e728b7c406ca77b0186e9a7686a3  WifiCommanderPentax_Installer.msi
```

`ilspycmd` 11.0 fails on it with `BadImageFormatException: Illegal tables in
compressed metadata stream` — the assembly is obfuscated or packed, which is the
likely reason the original attempt produced nothing. Symbol strings are still
readable and confirm the app has `FormBulb`, `camera_shoot_bulb`, `EMBULB`,
`EMBULB_TIMER`, `RealBULB`, `btnIntervalShot` and a `SMinterval*` family. That
tells us the vendor product separates a Bulb dialog and an intervalometer, which
is a product-behaviour hint only.

## Is it worth obtaining an Image Sync APK?

**Yes, but narrowly, and not as a PTP reference.** Justification:

- It is the Android counterpart of the same camera Wi-Fi service Wi-Fi Commander
  drives, and unlike that .NET assembly it is a normal DEX file: `apktool` or
  `jadx` decompile it cleanly, so unlike Wi-Fi Commander it will actually be
  readable.
- The Polaris integration is itself an Android app driving a camera over a
  network-ish transport, so Image Sync's *state machine* — how it decides a shot
  finished, how it handles busy, what it polls during a long exposure — is the
  closest available analogue to what `pgphoto` must do.
- It is cheap: one APK, hashed and archived like a FwPkt.

It will **not** answer opcode or offset questions, because it does not speak PTP.
If a PTP-level question is open, the answer is a USB trace or IT2, not an APK.
Recommended handling when obtained: archive to PrivateResearch with its SHA-256
and Play Store version, decompile with `jadx`, and record findings here rather
than in a patcher evidence directory.

## Rules for citing reference clients

1. Name the client, the file, the symbol, and the content hash. "IT2 does X"
   without a hash is unverifiable after a decompiler upgrade.
2. State the transport. A Wi-Fi-client fact cannot support a PTP claim.
3. Label it with the evidence classes used in `PENTAX_WIRE_PROTOCOL.md` —
   Observed-client / Inferred-client / Unknown-hardware. Client behaviour is
   evidence that a behaviour is *possible and interoperable*, never proof of what
   the camera requires.
4. A reference client is a floor, not a ceiling: matching IT2 is sufficient for
   compatibility testing and insufficient as a design goal.
