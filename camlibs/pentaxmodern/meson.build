pentaxmodern_sources = files('pentaxmodern.c', 'pentaxmodern.h')

glib_dep = dependency('glib-2.0', version: '>=2.32', required: true)
# libgphoto2_dep is expected to be available from the calling meson.build (camlibs/meson.build)

shared_module('gphoto2_pentaxmodern',
  pentaxmodern_sources,
  dependencies: [libgphoto2_dep, glib_dep],
  # Ensure consistent naming for the .so file
  name_prefix: '',
  install: true,
  install_dir: camlibs_dir
)
