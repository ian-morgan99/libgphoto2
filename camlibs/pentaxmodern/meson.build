pentaxmodern_sources = files('pentaxmodern.c')

incs = include_directories('.', '../../libgphoto2')

shared_module(
  'gphoto2_pentaxmodern',
  pentaxmodern_sources,
  install : true,
  include_directories : incs,
  c_args : [
    '-D_POSIX_C_SOURCE=200809L',  # Fix for usleep
    '-Wno-unused-parameter',      # Optional: suppress harmless warnings
    '-I../../libgphoto2'          # Ensure ptp.h can be found
  ]
)
