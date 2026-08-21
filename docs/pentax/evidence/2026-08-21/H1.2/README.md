# H1.2 — guarded vendor lifecycle and read-only descriptors

Camera serial is deliberately omitted. Camera: K-3 Mark III firmware 2.20.

## Safety envelope

The candidate was freshly compiled, explicitly selected, and bounded by an
outer timeout. Initialization sent the observed `SetVendorMode(enable)` command.
The summary performed reads only. Exit sent `SetVendorMode(disable)`. No device
property was written, and no live-view, capture, file, delete, reset, or firmware
operation was requested.

## Results

- Candidate initialization succeeded and vendor descriptors became visible.
- White balance `0x5005` is read/write UINT16 enumeration.
- F-number `0x5007` is read/write UINT16 enumeration.
- exposure compensation `0x5010` is read/write INT16 enumeration.
- shutter `0xd00f` is read/write UINT64 enumeration.
- USB live-view mode `0xd035` is read/write UINT8 enumeration `[0,1]`, current 0.
- standard ISO `0x500f` and focus mode `0x500a` are absent.
- vendor `0xd01e` is read/write UINT32 with an empty reported enumeration; its
  semantics are not inferred.
- An independent forced-generic session after candidate exit showed only the
  original five MTP properties. This proves disable restored pre-session state.

No values or image data are recorded here.
