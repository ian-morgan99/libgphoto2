# Pentax/Ricoh model sweep — public summary

Tracked by #52.

The sweep covers every Pentax/Ricoh body referenced by current libgphoto2 code, legacy `camlibs/pentax`, Image Transmitter research, `PentaxLibrary`, `nina-pentax-spec`, Benro Polaris evidence and existing hardware logs.

The important architectural split is by transport/protocol family rather than marketing brand alone:

- modern Pentax vendor/PTP bodies, with old/new focus subfamilies;
- legacy USB-SCSI/MSC bodies handled by `camlibs/pentax`;
- generic PTP/MTP personalities that may expose little or no Pentax vendor control surface;
- Ricoh/Pentax bodies represented in ptp2 with distinct property families.

For every model/mode the matrix must record VID:PID, transport, claiming camlib, VendorExtensionID, vendor-mode eligibility, capture/preview/focus family, generic control availability, Pentax-specific equivalents, and evidence level.

Do not infer support across bodies without explicit evidence. K-3 III, K-1 II and K-01 already demonstrate why apparently similar Pentax cameras require separate treatment.