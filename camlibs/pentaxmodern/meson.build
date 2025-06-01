pentaxmodern_sources = files('pentaxmodern.c', 'pentaxmodern.h')

glib_dep = dependency('glib-2.0', version: '>=2.32', required: true)

shared_module('gphoto2_pentaxmodern',
  pentaxmodern_sources,
  dependencies: [libgphoto2_dep, glib_dep],
  name_suffix: camlib_name_suffix,
  install: true,
  install_dir: camlibs_install_dir
)
