#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
HARNESS_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
SOURCE_ROOT=${LIBGPHOTO2_SOURCE:-$HARNESS_ROOT}
EXPECTED_SHA=67843d2248e7e37cfa58c15cc52e0a47bc13d952
ACTUAL_SHA=$(git -C "$SOURCE_ROOT" rev-parse HEAD)

if [ "$ACTUAL_SHA" != "$EXPECTED_SHA" ]; then
	printf 'refusing build: expected source %s, found %s\n' \
		"$EXPECTED_SHA" "$ACTUAL_SHA" >&2
	exit 2
fi
if [ "$#" -ne 1 ]; then
		printf 'usage: %s OUTPUT_WRAPPER\n' "$0" >&2
	exit 2
fi

BUILD_DIR=${LIBGPHOTO2_BUILD:-$SOURCE_ROOT/_build}
if [ ! -f "$BUILD_DIR/meson-uninstalled/libgphoto2-uninstalled.pc" ]; then
	printf 'missing configured Meson build: %s\n' "$BUILD_DIR" >&2
	exit 2
fi
for required in \
	"$BUILD_DIR/camlibs/ptp2/ptp2.so" \
	"$BUILD_DIR/libgphoto2_port/libusb1/usb1.so" \
	"$BUILD_DIR/libgphoto2_port/usbscsi/usbscsi.so"; do
	if [ ! -f "$required" ]; then
		printf 'missing exact-build plugin: %s\n' "$required" >&2
		exit 2
	fi
done

OUTPUT=$1
case "$OUTPUT" in
	/*) ;;
	*)
		printf 'output path must be absolute\n' >&2
		exit 2
		;;
esac
BINARY=${OUTPUT}.bin
IOLIBS_DIR=${OUTPUT}.iolibs
case "$OUTPUT$BINARY$IOLIBS_DIR$BUILD_DIR" in
		*"'"*|*'
'*)
			printf 'refusing paths containing a quote or newline\n' >&2
			exit 2
			;;
esac
if [ -e "$OUTPUT" ] || [ -e "$BINARY" ] || [ -e "$IOLIBS_DIR" ]; then
		printf 'refusing to overwrite existing probe outputs\n' >&2
		exit 2
fi

mkdir "$IOLIBS_DIR"
cp "$BUILD_DIR/libgphoto2_port/libusb1/usb1.so" "$IOLIBS_DIR/usb1.so"
cp "$BUILD_DIR/libgphoto2_port/usbscsi/usbscsi.so" "$IOLIBS_DIR/usbscsi.so"
export PKG_CONFIG_PATH="$BUILD_DIR/meson-uninstalled${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
CC=${CC:-cc}

"$CC" -D_GPHOTO2_INTERNAL_CODE -DHAVE_UNISTD_H=1 -DHAVE_CONFIG_H=1 \
		-o "$BINARY" \
		"$SCRIPT_DIR/pentax-bulb-timed-probe.c" \
		"$SCRIPT_DIR/pentax-bulb-timed-probe-guard.c" \
		"$SCRIPT_DIR/pentax-bulb-timed-probe-logshim.c" \
		"$SCRIPT_DIR/pentax-bulb-timed-probe-translate-shim.c" \
		"$SOURCE_ROOT/examples/autodetect.c" \
		"$SOURCE_ROOT/camlibs/ptp2/ptp.c" \
		"$SOURCE_ROOT/camlibs/ptp2/pentax-utils.c" \
		"$SOURCE_ROOT/camlibs/ptp2/ptpip.c" \
		-I"$BUILD_DIR" -I"$BUILD_DIR/libgphoto2" \
		-I"$BUILD_DIR/libgphoto2_port" \
		-I"$SOURCE_ROOT" -I"$SOURCE_ROOT/libgphoto2" \
		-I"$SOURCE_ROOT/libgphoto2_port" \
		-I"$SOURCE_ROOT/libgphoto2_port/gphoto2" \
		-I"$SOURCE_ROOT/libgphoto2_port/libgphoto2_port" \
		-I"$SOURCE_ROOT/camlibs/ptp2" -I"$SOURCE_ROOT/examples" \
		$(pkg-config --cflags --libs libgphoto2) \
		-L"$BUILD_DIR/libgphoto2" \
		-L"$BUILD_DIR/libgphoto2_port/libgphoto2_port" \
		-Wl,-rpath,"$BUILD_DIR/libgphoto2" \
		-Wl,-rpath,"$BUILD_DIR/libgphoto2_port/libgphoto2_port" \
		-lgphoto2 -lgphoto2_port

{
		printf '%s\n' '#!/bin/sh' 'set -eu'
		printf "export CAMLIBS='%s'\n" "$BUILD_DIR/camlibs/ptp2"
		printf "export IOLIBS='%s'\n" "$IOLIBS_DIR"
		printf "export LD_LIBRARY_PATH='%s/libgphoto2:%s/libgphoto2_port/libgphoto2_port'\n" \
			"$BUILD_DIR" "$BUILD_DIR"
		printf "exec '%s' \"\$@\"\n" "$BINARY"
} > "$OUTPUT"
chmod +x "$OUTPUT"

"$OUTPUT" --self-test
printf 'probe_built=%s source_sha=%s camlibs=%s iolibs=%s\n' \
		"$OUTPUT" "$ACTUAL_SHA" "$BUILD_DIR/camlibs/ptp2" "$IOLIBS_DIR"
