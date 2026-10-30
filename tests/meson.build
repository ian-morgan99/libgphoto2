gp_test_env = gp_port_test_env + [
  'CAMLIBS=@0@'.format(':'.join(camlib_paths)),
]

ptp_context_test = executable(
  'test-ptp-context',
  'test-ptp-context.c',
  '../camlibs/ptp2/ptp-context.c',
  include_directories: include_directories('..', '../camlibs/ptp2'),
  dependencies: [libgphoto2_dep, dependency('threads')],
)

test('test-ptp-context', ptp_context_test)

# Generic PTP/MTP object-parent regression: both the ordinary PTP root and
# the MTP storage-root sentinel terminate folder reconstruction.  This must
# remain independent of Pentax model and capture tests.
test_ptp_folder_root_exe = executable(
  'test-ptp-folder-root',
  'test-ptp-folder-root.c',
  include_directories: include_directories('..', '../camlibs/ptp2'),
  dependencies: libgphoto2_dep,
)

test('test-ptp-folder-root', test_ptp_folder_root_exe)

# Compile the unchanged production cleanup body with mocked transport; never
# open a camera or add a test entry point to the installed camlib ABI.
camera_exit_source = custom_target(
  'test-camera-exit-source',
  input: '../camlibs/ptp2/library.c',
  output: 'test-camera-exit.inc',
  command: [python, files('extract-camera-exit.py'), '@INPUT@', '@OUTPUT@'],
)
test_pentax_camera_exit = executable(
  'test-pentax-camera-exit',
  'test-pentax-camera-exit.c', camera_exit_source,
  include_directories: include_directories('..', '../camlibs/ptp2'),
  dependencies: libgphoto2_dep,
)
test('test-pentax-camera-exit', test_pentax_camera_exit)

pentax_session_source = custom_target(
  'test-pentax-session-source',
  input: '../camlibs/ptp2/library.c',
  output: 'test-pentax-session.inc',
  command: [python, files('extract-pentax-session.py'), '@INPUT@', '@OUTPUT@'],
  depend_files: files('extract-camera-exit.py'),
)
test_pentax_session = executable(
  'test-pentax-session',
  'test-pentax-session.c', pentax_session_source,
  '../camlibs/ptp2/pentax-utils.c',
  include_directories: include_directories('..', '../camlibs/ptp2'),
  dependencies: libgphoto2_dep,
)
test('test-pentax-session', test_pentax_session)

test_pentax_utils_exe = executable(
  'test-pentax-utils',
  'test-pentax-utils.c',
  '../camlibs/ptp2/pentax-utils.c',
  include_directories: include_directories('../camlibs/ptp2'),
  dependencies: libgphoto2_dep,
)

test(
  'test-pentax-utils',
  test_pentax_utils_exe,
  env: gp_test_env,
)

# Deterministic tests for the dual-format exposure reconciliation loop
# (issue #73): pentax_reconcile_extra_candidates() with mocked camera
# behaviour, covering one/two candidates, transfer/delete failures,
# transient conditions failures, bound exhaustion and cancellation.
test_pentax_reconcile_exe = executable(
  'test-pentax-reconcile',
  'test-pentax-reconcile.c',
  '../camlibs/ptp2/pentax-utils.c',
  include_directories: include_directories('../camlibs/ptp2'),
  dependencies: libgphoto2_dep,
)

test(
  'test-pentax-reconcile',
  test_pentax_reconcile_exe,
  env: gp_test_env,
)

test_pentax_legacy_parsers_exe = executable(
  'test-pentax-legacy-parsers',
  'test-pentax-legacy-parsers.c',
  '../camlibs/pentax/pslr_utils.c',
  '../camlibs/pentax/pslr_model.c',
  '../camlibs/pentax/pslr_log.c',
  '../camlibs/pentax/js0n.c',
  include_directories: include_directories('../camlibs/pentax'),
  c_args: ['-DPKTDATADIR="/"'],
  dependencies: [libgphoto2_dep, m_dep],
)

test('test-pentax-legacy-parsers', test_pentax_legacy_parsers_exe)

# Source-contract regression: ordinary Manual and camera-timed Bulb must use
# the explicit release=0 route; only the separately gated research action may
# use release=2, and only its explicit stop edge may call TerminateCapture.
test(
  'test-pentax-capture-contract',
  python,
  args: [
    files('test-pentax-capture-contract.py'),
    meson.project_source_root() / 'camlibs/ptp2/library.c',
  ],
)

# Unit tests use an injected camera API to verify that every failed Bulb path
# stops an opened shutter before cleanup or exits fail-closed without an
# unmatched stop when start was not confirmed.
test_pentax_bulb_lifecycle_exe = executable(
  'test-pentax-bulb-lifecycle',
  'test-pentax-bulb-lifecycle.c',
  'pentax-bulb-lifecycle.c',
  include_directories: include_directories('..'),
  dependencies: libgphoto2_dep,
)

test('test-pentax-bulb-lifecycle', test_pentax_bulb_lifecycle_exe)

# Manual direct-hardware probe only. It is built solely with the Pentax
# research-capture option and is never registered as an automated test.
if get_option('pentax_research_capture')
  executable(
    'pentax-bulb-qualification-probe',
    'pentax-bulb-qualification-probe.c',
    'pentax-bulb-lifecycle.c',
    '../examples/autodetect.c',
    include_directories: include_directories('..', '../camlibs/ptp2', '../examples'),
    c_args: ['-D_GPHOTO2_INTERNAL_CODE'],
    dependencies: libgphoto2_dep,
  )
endif

test_pentax_publication_exe = executable(
  'test-pentax-publication',
  'test-pentax-publication.c',
  '../camlibs/ptp2/pentax-publication.c',
  include_directories: [include_directories('../camlibs/ptp2'), include_directories('..')],
  dependencies: libgphoto2_dep,
)

test(
  'test-pentax-publication',
  test_pentax_publication_exe,
  env: gp_test_env,
)

# Inventory-only regression: the measured iPolar is UVC, while the measured
# 16c0:29a0 StarShoot is vendor-specific USB and must not be advertised as UVC.
test_uvc_device_table_exe = executable(
  'test-uvc-device-table',
  'test-uvc-device-table.c',
)

test(
  'test-uvc-device-table',
  test_uvc_device_table_exe,
)

# Regression test for Pentax generic exposure compatibility aliases.
# Drives the real public config-lookup path: dlopens the built ptp2 camlib via
# CAMLIBS and calls its exported camera_get_single_config("aperture") with a
# hand-built Camera fixture whose PTP state is pre-seeded (Pentax vendor mode +
# FNumber descriptor in the property cache), so no wire IO happens. Needs the
# ptp2 camlib headers for the private-data layout; '..' provides the build root
# for the generated libgphoto2/gphoto2-endian.h that ptp.h includes.
test_pentax_aperture_alias_exe = executable(
  'test-pentax-aperture-alias',
  'test-pentax-aperture-alias.c',
  include_directories: [include_directories('../camlibs/ptp2'), include_directories('..')],
  dependencies: [ libgphoto2_dep ],
)

test(
  'test-pentax-aperture-alias',
  test_pentax_aperture_alias_exe,
  env: gp_test_env,
)

test_i18n_exe = executable(
  'test-i18n',
  'test-i18n.c',
  dependencies: [ libgphoto2_dep, intl_dep ],
)

test(
  'test-i18n',
  test_i18n_exe,
  env: gp_test_env,
)

test_endian_exe = executable(
  'test-endian',
  'test-endian.c',
  dependencies: libgphoto2_dep,
)

test(
  'test-endian',
  test_endian_exe,
  env: gp_test_env,
)

print_libgphoto2_version_exe = executable(
  'print-libgphoto2-version',
  'print-libgphoto2-version.c',
  dependencies: libgphoto2_dep,
)

test(
  'print-libgphoto2-version',
  print_libgphoto2_version_exe,
  env: gp_test_env,
)

test_gphoto2_exe = executable(
  'test-gphoto2',
  'test-gphoto2.c',
  dependencies: libgphoto2_dep,
)

test(
  'test-gphoto2',
  test_gphoto2_exe,
  env: gp_test_env,
)

test_filesys_exe = executable(
  'test-filesys',
  'test-filesys.c',
  dependencies: libgphoto2_dep,
)

test(
  'test-filesys',
  test_filesys_exe,
  env: gp_test_env,
)

test_camera_list_exe = executable(
  'test-camera-list',
  'test-camera-list.c',
  dependencies: libgphoto2_dep,
)

test(
  'test-camera-list',
  test_camera_list_exe,
  env: gp_test_env,
)

test_init_localedir_exe = executable(
  'test-init-localedir',
  'test-init-localedir.c',
  dependencies: libgphoto2_dep,
)

test(
  'test-init-localedir',
  test_init_localedir_exe,
  env: gp_test_env,
)
