pentaxmodern_sources = files('pentaxmodern.c', 'pentaxmodern.h')

glib_dep = dependency('glib-2.0', version: '>=2.32', required: true)
# libgphoto2_dep and libgphoto2_root_inc are expected to be available
# from the calling meson.build (camlibs/meson.build or root meson.build)

shared_module('gphoto2_pentaxmodern',
  pentaxmodern_sources,
  dependencies: [libgphoto2_dep, glib_dep],
  include_directories: [libgphoto2_root_inc], # Explicitly add project root here
  # Ensure consistent naming for the .so file
  name_prefix: '',
  install: true,
  install_dir: camlibs_dir
)
