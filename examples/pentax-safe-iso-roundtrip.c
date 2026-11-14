/* Exact-model, single-session Pentax ISO live-baseline round trip. */
#include "config.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-widget.h>

#include "samples.h"

static int
parse_iso_value (const char *value, unsigned int *iso)
{
	char *end = NULL;
	unsigned long parsed;

	if (!value || !iso || !*value)
		return GP_ERROR_BAD_PARAMETERS;
	parsed = strtoul (value, &end, 10);
	if ((end == value) || (*end && (*end != ';')) ||
	    (parsed > UINT_MAX))
		return GP_ERROR_CORRUPTED_DATA;
	*iso = (unsigned int)parsed;
	return GP_OK;
}

static int
read_conditions_iso (Camera *camera, GPContext *context, unsigned int *iso)
{
	CameraWidget *widget = NULL;
	const char *value = NULL, *start;
	int result;

	result = gp_camera_get_single_config (camera, "pentaxconditions", &widget,
		context);
	if (result < GP_OK)
		return result;
	result = gp_widget_get_value (widget, &value);
	if (result >= GP_OK) {
		start = strstr (value, "ISO=");
		if (!start)
			result = GP_ERROR_CORRUPTED_DATA;
		else
			result = parse_iso_value (start + 4, iso);
	}
	gp_widget_free (widget);
	return result;
}

static int
read_iso_choices (Camera *camera, GPContext *context, unsigned int live_iso,
	char **baseline_value, char **target_value, unsigned int *target_iso)
{
	CameraWidget *widget = NULL;
	const char *value = NULL, *choice = NULL;
	unsigned int widget_iso, choice_iso;
	int count, i, current = -1, target = -1, result;

	result = gp_camera_get_single_config (camera, "pentaxdirectiso", &widget,
		context);
	if (result < GP_OK)
		return result;
	result = gp_widget_get_value (widget, &value);
	if (result < GP_OK)
		goto out;
	result = parse_iso_value (value, &widget_iso);
	if ((result < GP_OK) || (widget_iso != live_iso)) {
		result = GP_ERROR_CORRUPTED_DATA;
		goto out;
	}
	*baseline_value = strdup (value);
	if (!*baseline_value) {
		result = GP_ERROR_NO_MEMORY;
		goto out;
	}
	count = gp_widget_count_choices (widget);
	if (count < GP_OK) {
		result = count;
		goto out;
	}
	for (i = 0; i < count; i++) {
		result = gp_widget_get_choice (widget, i, &choice);
		if (result < GP_OK)
			goto out;
		if (parse_iso_value (choice, &choice_iso) < GP_OK)
			continue;
		if (choice_iso == live_iso)
			current = i;
	}
	if (current < 0) {
		result = GP_ERROR_CORRUPTED_DATA;
		goto out;
	}
	/* Prefer the adjacent lower advertised value, avoiding a guessed ISO. */
	target = current > 0 ? current - 1 : current + 1;
	if ((target < 0) || (target >= count)) {
		result = GP_ERROR_NOT_SUPPORTED;
		goto out;
	}
	result = gp_widget_get_choice (widget, target, &choice);
	if (result < GP_OK)
		goto out;
	result = parse_iso_value (choice, target_iso);
	if (result < GP_OK || *target_iso == live_iso) {
		result = GP_ERROR_CORRUPTED_DATA;
		goto out;
	}
	*target_value = strdup (choice);
	result = *target_value ? GP_OK : GP_ERROR_NO_MEMORY;
out:
	if (widget)
		gp_widget_free (widget);
	return result;
}

static int
wait_for_iso (Camera *camera, GPContext *context, unsigned int expected,
	unsigned int *actual)
{
	unsigned int attempt;
	int result;

	for (attempt = 0; attempt < 5; attempt++) {
		result = read_conditions_iso (camera, context, actual);
		if ((result >= GP_OK) && (*actual == expected))
			return GP_OK;
		if (attempt < 4)
			usleep (100000);
	}
	return GP_ERROR_TIMEOUT;
}

static int
set_iso (Camera *camera, GPContext *context, const char *value)
{
	CameraWidget *widget = NULL;
	int result;

	result = gp_camera_get_single_config (camera, "pentaxdirectiso", &widget,
		context);
	if (result < GP_OK)
		return result;
	result = gp_widget_set_value (widget, value);
	if (result >= GP_OK)
		result = gp_camera_set_single_config (camera, "pentaxdirectiso", widget,
			context);
	gp_widget_free (widget);
	return result;
}

int
main (int argc, char **argv)
{
	Camera *camera = NULL;
	GPContext *context = NULL;
	char *baseline_value = NULL, *target_value = NULL;
	unsigned int baseline_iso = 0, target_iso = 0, iso = 0;
	int initialized = 0, restore_required = 0;
	int result = GP_OK, set_result = GP_OK, wait_result = GP_OK;
	int restore_result = GP_OK, restore_wait = GP_OK, exit_result;

	if ((argc != 3) || strcmp (argv[1], "Pentax K-1 Mark II (PTP mode)")) {
		fprintf (stderr, "usage: %s 'Pentax K-1 Mark II (PTP mode)' usb:BUS,DEVICE\n",
			argv[0]);
		return 2;
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
	result = read_conditions_iso (camera, context, &baseline_iso);
	if (result < GP_OK)
		goto out;
	printf ("stage=initial conditions_iso=%u\n", baseline_iso);
	result = read_iso_choices (camera, context, baseline_iso, &baseline_value,
		&target_value, &target_iso);
	if (result < GP_OK)
		goto out;
	printf ("stage=advertised baseline=%s target=%s\n", baseline_value,
		target_value);

	/* The request may apply even if its response is lost; always attempt the
	 * live-baseline restore after a SET has been issued. */
	restore_required = 1;
	set_result = set_iso (camera, context, target_value);
	wait_result = wait_for_iso (camera, context, target_iso, &iso);
	printf ("stage=target set_result=%d wait_result=%d conditions_iso=%u\n",
		set_result, wait_result, iso);
	if ((set_result < GP_OK) || (wait_result < GP_OK) || (iso != target_iso))
		result = set_result < GP_OK ? set_result :
			(wait_result < GP_OK ? wait_result : GP_ERROR);

out:
	if (restore_required && camera && baseline_value) {
		restore_result = set_iso (camera, context, baseline_value);
		restore_wait = wait_for_iso (camera, context, baseline_iso, &iso);
		printf ("stage=restore set_result=%d wait_result=%d conditions_iso=%u\n",
			restore_result, restore_wait, iso);
		if ((restore_result < GP_OK) || (restore_wait < GP_OK) ||
		    (iso != baseline_iso))
			result = restore_result < GP_OK ? restore_result :
				(restore_wait < GP_OK ? restore_wait : GP_ERROR);
	}
	if (initialized) {
		exit_result = gp_camera_exit (camera, context);
		if ((result >= GP_OK) && (exit_result < GP_OK))
			result = exit_result;
	}
	if (camera)
		gp_camera_unref (camera);
	gp_context_unref (context);
	free (target_value);
	free (baseline_value);
	if (result < GP_OK) {
		fprintf (stderr, "ISO round trip failed: %s (%d); restore_result=%d\n",
			gp_result_as_string (result), result, restore_result);
		return 1;
	}
	printf ("iso_roundtrip=pass cleanup=ok\n");
	return 0;
}
