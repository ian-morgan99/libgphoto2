/*
 * Issue #141: measure Pentax config latency on one persistent session.
 *
 * This is an opt-in hardware benchmark. It never captures, never writes the
 * card, and restores focus peaking to its value at startup before exiting.
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
set_keep_live_view (Camera *camera, GPContext *context, const char *value)
{
	CameraWidget *widget = NULL;
	int result;

	result = gp_camera_get_single_config (camera, "pentaxpclvkeep", &widget,
		context);
	if (result >= GP_OK)
		result = gp_widget_set_value (widget, value);
	if (result >= GP_OK)
		result = gp_camera_set_single_config (camera, "pentaxpclvkeep", widget,
			context);
	if (widget)
		gp_widget_free (widget);
	return result;
}

static int
start_persistent_live_view (Camera *camera, GPContext *context)
{
	CameraFile *file = NULL;
	int result;

	result = set_keep_live_view (camera, context, "on");
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
	if (initialized && attempted) {
		double restore_seconds = 0.0, read_seconds = 0.0;
		int read_result = read_focus_peaking (camera, context, current,
		                                     sizeof current, &read_seconds);
		if (read_result >= GP_OK && strcmp (current, initial)) {
			int restore_result = set_focus_peaking (camera, context, initial,
			                                       full_mode, &restore_seconds);
			printf ("restore=attempt target=%s set_seconds=%.6f result=%d\n",
			        initial, restore_seconds, restore_result);
			if ((result >= GP_OK) && (restore_result < GP_OK))
				result = restore_result;
		} else {
			printf ("restore=not-needed current=%s read_seconds=%.6f result=%d\n",
			        current, read_seconds, read_result);
		}
	}
	if (initialized) {
		if (keep_live_view) {
			int keep_result = set_keep_live_view (camera, context, "off");
			printf ("live-view-keep=off result=%d\n", keep_result);
			if ((result >= GP_OK) && (keep_result < GP_OK))
				result = keep_result;
		}
		int exit_result = gp_camera_exit (camera, context);
		if ((result >= GP_OK) && (exit_result < GP_OK))
			result = exit_result;
	}
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
