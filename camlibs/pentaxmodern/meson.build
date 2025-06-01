pentaxmodern_sources = files('pentaxmodern.c')

incs = include_directories('.', '../../libgphoto2', meson.current_build_dir())

shared_module(
  'gphoto2_pentaxmodern',
  pentaxmodern_sources,
  install : true,
  include_directories : incs,
  dependencies: [libgphoto2_dep, libxml_dep, libcurl_dep],
  c_args : [
    '-Wno-unused-parameter',      # Optional: suppress harmless warnings
    '-I../../libgphoto2'          # Ensure ptp.h can be found
  ]
)
