/*
 * Issue #141: measure Pentax config latency on one persistent session.
 *
 * This is an opt-in hardware benchmark. It never captures and never writes the
 * card. Cleanup attempts to restore focus peaking to its value at startup and
 * verifies that by an independent readback; if the state cannot be verified it
 * reports final_value_verified=UNKNOWN or MISMATCH and exits non-zero.
 * The default "single" mode is the Polaris-relevant API path. "full" mode
 * measures the legacy full-tree path used by the gphoto2 CLI today.
 * "single-lv" and "full-lv" first start one PC-LV preview in memory and keep
 * the session active; this is required for Live-View-scoped Pentax controls.
 */
#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-file.h>
#include <gphoto2/gphoto2-widget.h>

#include "samples.h"

#define CONFIG_NAME "pentaxfocuspeaking"
#define DEFAULT_ITERATIONS 4
#define MAX_ITERATIONS 20

static double
monotonic_seconds (void)
{
	struct timespec now;

	if (clock_gettime (CLOCK_MONOTONIC, &now) != 0)
		return 0.0;
	return (double) now.tv_sec + ((double) now.tv_nsec / 1000000000.0);
}

static int
read_focus_peaking (Camera *camera, GPContext *context, char *value,
	                    size_t value_size, double *elapsed)
{
	CameraWidget *widget = NULL;
	const char *text = NULL;
	double started = monotonic_seconds ();
	int result;

	result = gp_camera_get_single_config (camera, CONFIG_NAME, &widget, context);
	if (result >= GP_OK)
		result = gp_widget_get_value (widget, &text);
	if ((result >= GP_OK) && (!text || !*text ||
	                          strlen (text) >= value_size))
		result = GP_ERROR_CORRUPTED_DATA;
	if (result >= GP_OK)
		strcpy (value, text);
	if (elapsed)
		*elapsed = monotonic_seconds () - started;
	if (widget)
		gp_widget_free (widget);
	return result;
}

static int
set_keep_live_view (Camera *camera, GPContext *context, int enabled)
{
	/* pentaxpclvkeep is a GP_WIDGET_TOGGLE: _get_Pentax_KeepLiveView() creates it
	 * with gp_widget_new(GP_WIDGET_TOGGLE, ...) and _put_Pentax_KeepLiveView()
	 * reads it with gp_widget_get_value(widget, &int).  gp_widget_set_value
	 * dereferences its argument as int* for a toggle, so passing a string literal
	 * here reads the bytes of the pointer target instead: "off" is nonzero and
	 * would silently set keep_live_view=1 while reporting success.  Pass an int. */
	CameraWidget *widget = NULL;
	int result;

	result = gp_camera_get_single_config (camera, "pentaxpclvkeep", &widget,
		context);
	if (result >= GP_OK)
		result = gp_widget_set_value (widget, &enabled);
	if (result >= GP_OK)
		result = gp_camera_set_single_config (camera, "pentaxpclvkeep", widget,
			context);
	if (widget)
		gp_widget_free (widget);
	return result;
}

static int
read_keep_live_view (Camera *camera, GPContext *context, int *enabled)
{
	CameraWidget *widget = NULL;
	int result;

	result = gp_camera_get_single_config (camera, "pentaxpclvkeep", &widget,
		context);
	if (result >= GP_OK)
		result = gp_widget_get_value (widget, enabled);
	if (widget)
		gp_widget_free (widget);
	return result;
}

static int
start_persistent_live_view (Camera *camera, GPContext *context)
{
	CameraFile *file = NULL;
	int result;

	result = set_keep_live_view (camera, context, 1);
	if (result < GP_OK)
		return result;
	result = gp_file_new (&file);
	if (result >= GP_OK)
		result = gp_camera_capture_preview (camera, file, context);
	if (file)
		gp_file_unref (file);
	return result;
}

static int
set_focus_peaking_single (Camera *camera, GPContext *context,
	                         const char *value, double *elapsed)
{
	CameraWidget *widget = NULL;
	double started = monotonic_seconds ();
	int result;

	result = gp_camera_get_single_config (camera, CONFIG_NAME, &widget, context);
	if (result >= GP_OK)
		result = gp_widget_set_value (widget, value);
	if (result >= GP_OK)
		result = gp_camera_set_single_config (camera, CONFIG_NAME, widget,
		                                     context);
	if (elapsed)
		*elapsed = monotonic_seconds () - started;
	if (widget)
		gp_widget_free (widget);
	return result;
}

static int
set_focus_peaking_full (Camera *camera, GPContext *context, const char *value,
	                       double *elapsed)
{
	CameraWidget *root = NULL, *widget = NULL;
	double started = monotonic_seconds ();
	int result;

	result = gp_camera_get_config (camera, &root, context);
	if (result >= GP_OK)
		result = gp_widget_get_child_by_name (root, CONFIG_NAME, &widget);
	if (result >= GP_OK)
		result = gp_widget_set_value (widget, value);
	if (result >= GP_OK)
		result = gp_camera_set_config (camera, root, context);
	if (elapsed)
		*elapsed = monotonic_seconds () - started;
	if (root)
		gp_widget_free (root);
	return result;
}

static int
set_focus_peaking (Camera *camera, GPContext *context, const char *value,
	                   int full_mode, double *elapsed)
{
	if (full_mode)
		return set_focus_peaking_full (camera, context, value, elapsed);
	return set_focus_peaking_single (camera, context, value, elapsed);
}

static int
parse_iterations (const char *text, int *iterations)
{
	char *end = NULL;
	long parsed;

	errno = 0;
	parsed = strtol (text, &end, 10);
	if (errno || !end || *end || parsed < 1 || parsed > MAX_ITERATIONS)
		return GP_ERROR_BAD_PARAMETERS;
	*iterations = (int) parsed;
	return GP_OK;
}

int
main (int argc, char **argv)
{
	Camera *camera = NULL;
	GPContext *context = NULL;
	char initial[32] = { 0 }, current[32] = { 0 }, target[32];
	int iterations = DEFAULT_ITERATIONS, initialized = 0, result = GP_OK;
	int attempted = 0, full_mode = 0, live_view_mode = 0;
	int keep_live_view = 0, i;
	/* Cleanup reports operation_result, restore_result and final_value_verified
	 * separately: a failed primary operation must not hide a failed restore, and
	 * a restore is never claimed without an independent readback. */
	int operation_result = GP_OK, restore_result = GP_OK;
	/* Kept out of the verification block so a failed readback can be propagated
	 * as the exit status instead of being reduced to the string "UNKNOWN". */
	int verify_result = GP_OK;
	const char *final_value_verified = "not-attempted";

	if ((argc < 3) || (argc > 5) ||
	    strcmp (argv[1], "Pentax K-1 Mark II (PTP mode)")) {
		fprintf (stderr,
		         "usage: %s 'Pentax K-1 Mark II (PTP mode)' usb:BUS,DEVICE [ITERATIONS] [single|full]\n",
		         argv[0]);
		return 2;
	}
	if (argc >= 4 && parse_iterations (argv[3], &iterations) < GP_OK) {
		fprintf (stderr, "ITERATIONS must be in the range 1..%d\n",
		         MAX_ITERATIONS);
		return 2;
	}
	if (argc == 5) {
		if (!strcmp (argv[4], "full"))
			full_mode = 1;
		else if (!strcmp (argv[4], "single"))
			;
		else if (!strcmp (argv[4], "single-lv"))
			live_view_mode = 1;
		else if (!strcmp (argv[4], "full-lv")) {
			full_mode = 1;
			live_view_mode = 1;
		} else {
			fprintf (stderr,
			         "MODE must be single, full, single-lv or full-lv\n");
			return 2;
		}
	}

	context = sample_create_context ();
	if (!context)
		return 1;
	result = sample_open_camera (&camera, argv[1], argv[2], context);
	if (result < GP_OK)
		goto out;
	result = gp_camera_init (camera, context);
	if (result < GP_OK)
		goto out;
	initialized = 1;

	if (live_view_mode) {
		/* Set the cleanup flag first: even a failed preview must clear the
		 * session-local keep-live-view request before gp_camera_exit. */
		keep_live_view = 1;
		result = start_persistent_live_view (camera, context);
		if (result < GP_OK) {
			fprintf (stderr, "stage=start-persistent-lv error=%s (%d)\n",
			         gp_result_as_string (result), result);
			goto out;
		}
		printf ("stage=start-persistent-lv ok capture=preview-memory-only\n");
	}

	{
		double read_seconds;
		result = read_focus_peaking (camera, context, initial, sizeof initial,
		                             &read_seconds);
		if (result < GP_OK)
			goto out;
		snprintf (current, sizeof current, "%s", initial);
		printf ("issue=141 model=Pentax-K-1-II port=%s iterations=%d mode=%s\n",
		        argv[2], iterations,
		        live_view_mode ? (full_mode ? "full-lv" : "single-lv") :
		        (full_mode ? "full" : "single"));
		printf ("initial=%s read_seconds=%.6f persistent_session=yes capture=no\n",
		        initial, read_seconds);
	}

	for (i = 0; i < iterations; i++) {
		const char *next = (i % 2 == 0) ? "off" : "on";
		double set_seconds, read_seconds;

		if (!strcmp (current, next))
			next = !strcmp (next, "off") ? "on" : "off";
		strncpy (target, next, sizeof target - 1);
		target[sizeof target - 1] = '\0';
		attempted = 1;
		result = set_focus_peaking (camera, context, target, full_mode,
		                            &set_seconds);
		if (result < GP_OK) {
			fprintf (stderr, "iteration=%d target=%s set_error=%s (%d)\n",
			         i + 1, target, gp_result_as_string (result), result);
			goto out;
		}
		result = read_focus_peaking (camera, context, current, sizeof current,
		                             &read_seconds);
		printf ("iteration=%d target=%s set_seconds=%.6f read_seconds=%.6f "
		        "readback=%s\n", i + 1, target, set_seconds, read_seconds,
		        (result >= GP_OK && !strcmp (current, target)) ? "pass" : "fail");
		if ((result < GP_OK) || strcmp (current, target)) {
			if (result >= GP_OK)
				result = GP_ERROR_CORRUPTED_DATA;
			goto out;
		}
	}

out:
	operation_result = result;
	if (initialized && attempted) {
		double restore_seconds = 0.0, read_seconds = 0.0;
		int read_result = read_focus_peaking (camera, context, current,
		                                     sizeof current, &read_seconds);
		/* A failed read after a write leaves the post-write state unknown, and
		 * unknown is not the same as unchanged: restore best-effort to the one
		 * value known to have been live at startup. */
		int state_known = (read_result >= GP_OK);
		if (!state_known || strcmp (current, initial)) {
			restore_result = set_focus_peaking (camera, context, initial,
			                                   full_mode, &restore_seconds);
			printf ("restore=attempt reason=%s target=%s set_seconds=%.6f "
			        "result=%d\n", state_known ? "differs" : "state-unknown",
			        initial, restore_seconds, restore_result);
		} else {
			printf ("restore=not-needed current=%s read_seconds=%.6f result=%d\n",
			        current, read_seconds, read_result);
		}
		/* Verify independently: the restore write's own return code is not
		 * evidence that the camera holds the startup value. */
		{
			char verified[32] = { 0 };
			verify_result = read_focus_peaking (camera, context, verified,
			                                     sizeof verified, NULL);
			if (verify_result < GP_OK)
				final_value_verified = "UNKNOWN";
			else if (strcmp (verified, initial))
				final_value_verified = "MISMATCH";
			else
				final_value_verified = "initial";
			printf ("verify=%s read_result=%d\n", final_value_verified,
			        verify_result);
		}
	}
	if (initialized) {
		if (keep_live_view) {
			int keep_result = set_keep_live_view (camera, context, 0);
			int keep_read = 0;
			int keep_verify;

			printf ("live-view-keep=off result=%d\n", keep_result);
			/* Read the toggle back.  gp_camera_exit may also restore d035, but
			 * that does not make this write's return code truthful. */
			keep_verify = read_keep_live_view (camera, context, &keep_read);
			printf ("live-view-keep-readback=%d result=%d\n", keep_read,
			        keep_verify);
			if ((keep_result >= GP_OK) && (keep_verify < GP_OK))
				keep_result = keep_verify;
			else if ((keep_result >= GP_OK) && (keep_read != 0))
				keep_result = GP_ERROR_CORRUPTED_DATA;
			if ((result >= GP_OK) && (keep_result < GP_OK))
				result = keep_result;
		}
		int exit_result = gp_camera_exit (camera, context);
		if ((result >= GP_OK) && (exit_result < GP_OK))
			result = exit_result;
	}
	printf ("operation_result=%d restore_result=%d final_value_verified=%s\n",
	        operation_result, restore_result, final_value_verified);
	/* A benchmark that finished but left the camera in an unverified state is not
	 * a pass. */
	if ((result >= GP_OK) && (restore_result < GP_OK))
		result = restore_result;
	else if ((result >= GP_OK) && (attempted) &&
	         !strcmp (final_value_verified, "MISMATCH"))
		result = GP_ERROR_CORRUPTED_DATA;
	/* "UNKNOWN" is only ever set when the readback itself failed, so
	 * verify_result already carries the reason. An unverified camera state is
	 * not a pass: the whole point of the independent readback is that the
	 * restore write's own return code proves nothing about the camera. */
	else if ((result >= GP_OK) && (attempted) &&
	         !strcmp (final_value_verified, "UNKNOWN"))
		result = verify_result;
	if (camera)
		gp_camera_unref (camera);
	if (context)
		gp_context_unref (context);
	if (result < GP_OK) {
		fprintf (stderr, "persistent_config_benchmark=fail error=%s (%d)\n",
		         gp_result_as_string (result), result);
		return 1;
	}
	printf ("persistent_config_benchmark=pass cleanup=ok\n");
	return 0;
}
