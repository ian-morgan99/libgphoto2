# H1.1 — K-3 Mark III safe discovery

Camera serial is deliberately omitted. Date: 2026-08-21.

## Safety envelope

Only Linux USB enumeration, the existing GVFS MTP root, and a forced generic
`USB PTP Class Camera` summary were read. The generic driver prevented candidate
Pentax initialization. No set-property, vendor, capture, delete, download,
reset, or firmware operation was sent.

## Observations

- MTP mode USB identity: `25fb:0189`, Imaging interface, 480 Mbit/s.
- CD-ROM mode USB identity: `25fb:018a`, Mass Storage interface.
- Manufacturer: `RICOH IMAGING COMPANY, LTD.`
- exact DeviceInfo model: `PENTAX K-3 Mark III`
- device firmware/version: `2.20`
- PTP standard: 1.00
- VendorExtensionID: Microsoft `0x00000006`, version 1.10
- storage roots: `SD1` and `SD2`; generic summary enumerated SD1 successfully.
- pre-vendor DeviceInfo advertises standard MTP operations only, no capture and
  no `0x900x` Pentax operations.

The official K-3 III documentation also instructs IMAGE Transmitter 2 users to
select MTP. CD-ROM mode exposes the bundled software installer.

## Consequence

The previous `018c` and Pentax-VendorExtensionID assumptions were contradicted.
The candidate must use `0189`, avoid duplicate registration in the legacy
USB-SCSI camlib, and gate vendor routing on exact verified model identity.
