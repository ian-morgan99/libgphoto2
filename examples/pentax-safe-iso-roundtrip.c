/* Exact-model, single-session K-1 II ISO 200 -> 400 -> 200 diagnostic. */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-widget.h>

#include "samples.h"

static int
read_conditions_iso (Camera *camera, GPContext *context, unsigned int *iso)
{
	CameraWidget *widget = NULL;
	const char *value = NULL, *start;
	char *end = NULL;
	unsigned long parsed;
	int result;

	result = gp_camera_get_single_config (camera, "pentaxconditions", &widget,
		context);
	if (result < GP_OK)
		return result;
	result = gp_widget_get_value (widget, &value);
	if (result < GP_OK)
		goto out;
	start = strstr (value, "ISO=");
	if (!start) {
		result = GP_ERROR_CORRUPTED_DATA;
		goto out;
	}
	parsed = strtoul (start + 4, &end, 10);
	if ((end == start + 4) || (*end != ';') || (parsed > 0xffffffffUL)) {
		result = GP_ERROR_CORRUPTED_DATA;
		goto out;
	}
	*iso = (unsigned int)parsed;
out:
	gp_widget_free (widget);
	return result;
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
	unsigned int iso = 0;
	int initialized = 0, changed = 0;
	int result = GP_OK, set_result, restore_result = GP_OK, exit_result;

	if ((argc != 3) || strcmp (argv[1], "Pentax:K-1 Mark II (PTP mode)")) {
		fprintf (stderr, "usage: %s 'Pentax:K-1 Mark II (PTP mode)' usb:BUS,DEVICE\n",
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
	result = read_conditions_iso (camera, context, &iso);
	if (result < GP_OK)
		goto out;
	printf ("stage=initial conditions_iso=%u\n", iso);
	if (iso != 200U) {
		result = GP_ERROR_BAD_PARAMETERS;
		goto out;
	}

	set_result = set_iso (camera, context, "400");
	result = read_conditions_iso (camera, context, &iso);
	if (result < GP_OK)
		goto out;
	printf ("stage=target set_result=%d conditions_iso=%u\n", set_result, iso);
	changed = iso != 200U;
	if ((set_result < GP_OK) && !changed) {
		result = set_result;
		goto out;
	}
	if ((set_result < GP_OK) || (iso != 400U))
		result = GP_ERROR;

	if (changed) {
		restore_result = set_iso (camera, context, "200");
		if (read_conditions_iso (camera, context, &iso) < GP_OK)
			iso = 0;
		printf ("stage=restore set_result=%d conditions_iso=%u\n",
			restore_result, iso);
		if ((restore_result < GP_OK) || (iso != 200U))
			result = restore_result < GP_OK ? restore_result : GP_ERROR;
	}

out:
	if (initialized) {
		exit_result = gp_camera_exit (camera, context);
		if ((result >= GP_OK) && (exit_result < GP_OK))
			result = exit_result;
	}
	if (camera)
		gp_camera_unref (camera);
	gp_context_unref (context);
	if (result < GP_OK) {
		fprintf (stderr, "ISO round trip failed: %s (%d); changed=%d; "
			"restore_result=%d\n", gp_result_as_string (result), result,
			changed, restore_result);
		return 1;
	}
	printf ("iso_roundtrip=pass cleanup=ok\n");
	return 0;
}
