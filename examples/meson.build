executable(
  'sample-autodetect',
  'sample-autodetect.c',
  'samples.h',
  'context.c',
  'config.c',
  dependencies: libgphoto2_dep,
)

executable(
  'sample-multi-detect',
  'sample-multi-detect.c',
  'samples.h',
  'autodetect.c',
  'context.c',
  'config.c',
  dependencies: libgphoto2_dep,
)

executable(
  'lunkwill-canon-capture',
  'lunkwill-canon-capture.c',
  'samples.h',
  'context.c',
  'config.c',
  dependencies: libgphoto2_dep,
)

executable(
  'sample-owner',
  'sample-owner.c',
  'samples.h',
  'context.c',
  'config.c',
  dependencies: libgphoto2_dep,
)

executable(
  'sample-photobooth',
  'sample-photobooth.c',
  'samples.h',
  'context.c',
  'config.c',
  dependencies: libgphoto2_dep,
)

executable(
  'sample-preview',
  'preview.c',
  'samples.h',
  'context.c',
  'config.c',
  'focus.c',
  dependencies: libgphoto2_dep,
)

executable(
  'pentax-safe-preview',
  'pentax-safe-preview.c',
  'samples.h',
  'autodetect.c',
  'context.c',
  dependencies: libgphoto2_dep,
)

executable(
  'pentax-safe-iso-roundtrip',
  'pentax-safe-iso-roundtrip.c',
  'samples.h',
  'autodetect.c',
  'context.c',
  dependencies: libgphoto2_dep,
)

executable(
  'pentax-safe-shutter-roundtrip',
  'pentax-safe-shutter-roundtrip.c',
  'samples.h',
  'autodetect.c',
  'context.c',
  dependencies: libgphoto2_dep,
)

executable(
  'pentax-safe-focus',
  'pentax-safe-focus.c',
  'samples.h',
  'autodetect.c',
  'context.c',
  dependencies: libgphoto2_dep,
)

executable(
  'pentax-safe-lvaf-roundtrip',
  'pentax-safe-lvaf-roundtrip.c',
  'samples.h',
  'autodetect.c',
  'context.c',
  dependencies: libgphoto2_dep,
)

# Opt-in hardware benchmark for issue #141. It intentionally is not a Meson
# test: it toggles a live camera setting, restores the original value, and
# requires an explicitly selected camera and USB port.
executable(
  'pentax-persistent-config-benchmark',
  'pentax-persistent-config-benchmark.c',
  'samples.h',
  'autodetect.c',
  'context.c',
  dependencies: libgphoto2_dep,
)

executable(
  'sample-tether',
  'sample-tether.c',
  'samples.h',
  'context.c',
  'config.c',
  dependencies: libgphoto2_dep,
)

executable(
  'sample-afl',
  'sample-afl.c',
  'samples.h',
  'context.c',
  'autodetect.c',
  dependencies: libgphoto2_dep,
)

executable(
  'best-iso',
  'best-iso.c',
  dependencies: libgphoto2_dep,
)

executable(
  'sample-capture',
  'sample-capture.c',
  'samples.h',
  'context.c',
  'config.c',
  'focus.c',
  dependencies: libgphoto2_dep,
)

executable(
  'sample-trigger-capture',
  'sample-trigger-capture.c',
  'samples.h',
  'context.c',
  'config.c',
  dependencies: libgphoto2_dep,
)

executable(
  'sample-justfocus',
  'sample-justfocus.c',
  'samples.h',
  'context.c',
  'focus.c',
  dependencies: libgphoto2_dep,
)
