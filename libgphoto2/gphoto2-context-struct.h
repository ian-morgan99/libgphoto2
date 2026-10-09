/**
 * \file
 *
 * The real definition of struct _GPContext.
 *
 * It lives in a header rather than inside gphoto2-context.c so that anything
 * which legitimately needs the layout (the core, and the #190 regression test)
 * reads the same definition. Before this existed, both the ptp2 probe and the
 * test carried hand-maintained mirrors of the field order, and those mirrors
 * would have kept compiling unchanged if the real struct moved -- which is
 * precisely the failure benro-polaris-firmware-patcher#190 is about. There is
 * now exactly one definition, so there is nothing to drift.
 *
 * This is a private header: GPContext stays opaque in the public API. Do not
 * install it.
 **/
#include <gphoto2/gphoto2-context.h>

struct _GPContext
{
	GPContextIdleFunc     idle_func;
	void                 *idle_func_data;

	GPContextProgressStartFunc  progress_start_func;
	GPContextProgressUpdateFunc progress_update_func;
	GPContextProgressStopFunc   progress_stop_func;
	void                       *progress_func_data;

	GPContextErrorFunc    error_func;
	void                 *error_func_data;

	GPContextQuestionFunc question_func;
	void                 *question_func_data;

	GPContextCancelFunc   cancel_func;
	void                 *cancel_func_data;

	GPContextStatusFunc   status_func;
	void                 *status_func_data;

	GPContextMessageFunc  message_func;
	void                 *message_func_data;

	/* Reference count. The type and position are load-bearing: the closed
	 * Polaris app statically links its own copy of gp_context_new/ref/unref
	 * and decrements this field directly, so changing either would silently
	 * disagree with already-shipped machine code. Access it through the
	 * __atomic_* helpers in gphoto2-context.c, never with ++/--. */
	unsigned int ref_count;
};
