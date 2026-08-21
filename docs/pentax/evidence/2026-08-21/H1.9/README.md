# H1.9 — K-1 Mark II MSC read-only baseline

Camera: PENTAX K-1 Mark II, firmware descriptor 1.02, USB mode MSC. Serial is
intentionally omitted.

Only USB descriptors, Linux block-device metadata, directory entries, file
metadata, the first-file signature, and file contents for SHA-256 were read. No
SCSI command was issued directly by the test, and no file was created, changed,
deleted, renamed, or opened for writing.

## Identity and topology

- USB identity: `25fb:0182`.
- Interface: class/subclass/protocol `08/06/50` (Mass Storage, SCSI transparent,
  Bulk-Only).
- Linux exposed two LUN-backed block devices: a 59.5 GiB populated card with one
  exFAT partition, and a zero-capacity second LUN consistent with an empty card
  slot.
- The populated volume was automatically mounted read-write by the desktop
  before testing. All commands used here were read-only.
- The volume label was `K-3 III`, demonstrating that a label cannot identify
  the attached camera body. USB/udev parent identity must be used.

## File discovery and integrity baseline

- The newest supported media file by filesystem modification time was
  `DCIM/460_0821/K1II8664.DNG`.
- Size: 38,468,076 bytes.
- SHA-256:
  `db9ea0c3f402fe6b8d5ad03ca8bcce7d3249ffe073fed7776401d44c1dbdb1a5`.
- A signature-only inspection identified little-endian TIFF/DNG data and the
  embedded make/model as RICOH IMAGING COMPANY, LTD. / PENTAX K-1 Mark II.
- Media inventory under DCIM at this point: 1,183 DNG, 25 JPG, and 3 MOV files.

The preferred EXIF utility was not installed, so lens and detailed exposure
metadata were not extracted in this test. Do not infer them from filenames or
the volume label.

## Consequences

MSC discovery must enumerate every LUN, ignore zero-capacity/unmounted slots,
resolve each usable filesystem to its USB parent, and select the newest file by
timestamp with deterministic tie-breaking. A future PTP download of this exact
file can be compared with the hash above; neither backend may delete the source
as part of that comparison.
