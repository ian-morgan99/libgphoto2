/* Issue #190: build-time guard for the raw GPContext offsets used by the
 * opt-in context-lifetime probe in ptp-context.c.
 *
 * The probe reads GPContext fields by raw offset because GPContext is opaque
 * outside the core. That is only valid while the offsets hold on the 32-bit
 * ARM EABI target. They were derived twice over and agree:
 *   - struct _GPContext in libgphoto2/gphoto2-context.c (16 pointers, then
 *     ref_count) gives progress_start_func at 8 and ref_count at 64;
 *   - the shipped app's own gp_context_set_progress_funcs stores the start
 *     callback at [r3,#8], and its gp_context_unref decrements [r3,#64].
 *
 * Compile this on the target ABI to fail the build if the layout ever moves:
 *   arm-linux-gnueabi-gcc -fsyntax-only camlibs/ptp2/probe-offsets-check.c
 * On a 64-bit host the pointers are 8 bytes, so the assertions are skipped
 * rather than failing for the wrong reason.
 */
#include <stddef.h>

#if defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 4)

/* Mirror of struct _GPContext field order. */
struct gp_context_layout_probe {
	void *idle_func;
	void *idle_func_data;
	void *progress_start_func;
	void *progress_update_func;
	void *progress_stop_func;
	void *progress_func_data;
	void *error_func;
	void *error_func_data;
	void *question_func;
	void *question_func_data;
	void *cancel_func;
	void *cancel_func_data;
	void *status_func;
	void *status_func_data;
	void *message_func;
	void *message_func_data;
	unsigned int ref_count;
};

_Static_assert(__builtin_offsetof(struct gp_context_layout_probe, progress_start_func) == 8,
	       "GPContext.progress_start_func moved; update PROBE_OFF_PROGRESS_START");
_Static_assert(__builtin_offsetof(struct gp_context_layout_probe, progress_update_func) == 12,
	       "GPContext.progress_update_func moved; update PROBE_OFF_PROGRESS_UPDATE");
_Static_assert(__builtin_offsetof(struct gp_context_layout_probe, progress_stop_func) == 16,
	       "GPContext.progress_stop_func moved; update PROBE_OFF_PROGRESS_STOP");
_Static_assert(__builtin_offsetof(struct gp_context_layout_probe, progress_func_data) == 20,
	       "GPContext.progress_func_data moved; update PROBE_OFF_PROGRESS_DATA");
_Static_assert(__builtin_offsetof(struct gp_context_layout_probe, ref_count) == 64,
	       "GPContext.ref_count moved; update PROBE_OFF_REF_COUNT");

#endif

int ptp2_probe_offsets_check_translation_unit_exists;
