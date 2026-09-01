#!/bin/bash
# Build recipe for the standalone supervised bulb probe.
# Links ptp.c (which #includes ptp-pack.c), pentax-utils.c and ptpip.c
# directly; GP_LOG_E / translate_ptp_result come from tiny shims so we
# don't drag in library.c/config.c.
B=/home/ian/Documents/VSCodeProjects/LibGphoto2/libgphoto2/_build
SRC=/home/ian/Documents/VSCodeProjects/LibGphoto2/libgphoto2
T=$(dirname "$(readlink -f "$0")")
gcc -D_GPHOTO2_INTERNAL_CODE -o "$1" \
    "$T/cardwrite_probe.c" \
    "$SRC/camlibs/ptp2/ptp.c" \
    "$SRC/camlibs/ptp2/pentax-utils.c" \
    "$SRC/camlibs/ptp2/ptpip.c" \
    "$T/logshim.c" "$T/translate_shim.c" \
    -I$B -I$B/libgphoto2 -I$B/libgphoto2_port \
    -I$SRC -I$SRC/libgphoto2 -I$SRC/libgphoto2_port \
    -I$SRC/libgphoto2_port/gphoto2 -I$SRC/libgphoto2_port/libgphoto2_port \
    -I$SRC/camlibs/ptp2 \
    $(pkg-config --cflags libgphoto2) \
    -L$B/libgphoto2 -L$B/libgphoto2_port/libgphoto2_port \
    -lgphoto2 -lgphoto2_port
