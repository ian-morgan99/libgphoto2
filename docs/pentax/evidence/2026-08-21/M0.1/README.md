# M0.1 baseline record — 2026-08-21

Status: **BLOCKED for hardware; PASS for repository/tool inventory**

## Repositories

- libgphoto2 start commit: `9ca5375950853cc5ccc348b5b96ce6da925f3b79`
- libgphoto2 branch: `master`
- libgphoto2 origin: `https://github.com/ian-morgan99/libgphoto2.git`
- Polaris start commit: `8a31a0161301425c411a8be3a86e7962c6a03528`
- Polaris branch: `main`
- Polaris origin:
  `https://github.com/ian-morgan99/benro-polaris-firmware-patcher.git`
- Existing user edits at start: the two Pentax USB ability entries in
  `camlibs/ptp2/library.c`; the obsolete Polaris `DEVELOPMENT_PLAN.md` had been
  moved to the workspace archive.

## Tools

- Host compiler: GCC 13.3.0
- Docker: 29.1.3
- Host Meson: unavailable
- Host gphoto2 CLI: unavailable
- Build container: Ubuntu 24.04, current pip Meson (at least 1.4), Ninja,
  GCC, libusb-1.0, libltdl, libxml2, libjpeg, libexif, readline, gettext

## Camera inventory

`lsusb` contained no device with Pentax/Ricoh VID `25fb` and neither target
PID `018c` nor `0183`. It showed only host controllers, internal devices, a
Toshiba disk, and a Genesys Logic hub. Camera model, firmware, USB mode, serial,
and protocol responses are therefore unavailable.

Hardware acceptance is BLOCKED until a target camera is attached. No substitute
body and no synthetic response is treated as evidence.

