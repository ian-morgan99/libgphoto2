/* Exact-model, single-session K-1 II shutter 1/500 -> 1/125 -> 1/500. */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-widget.h>

#include "samples.h"

static int
read_shutter (Camera *camera, GPContext *context, unsigned int *num,
	unsigned int *den)
{
	CameraWidget *widget = NULL;
	const char *value = NULL, *start;
	int result;

	result = gp_camera_get_single_config (camera, "pentaxconditions", &widget,
		context);
	if (result < GP_OK)
		return result;
	result = gp_widget_get_value (widget, &value);
	if (result < GP_OK)
		goto out;
	start = strstr (value, "bulb-seconds=");
	if (!start || (sscanf (start, "bulb-seconds=%u/%u", num, den) != 2) ||
	    !*den)
		result = GP_ERROR_CORRUPTED_DATA;
out:
	gp_widget_free (widget);
	return result;
}

static int
set_shutter (Camera *camera, GPContext *context, const char *value)
{
	CameraWidget *widget = NULL;
	int result;

	result = gp_camera_get_single_config (camera, "pentaxdirectshutter", &widget,
		context);
	if (result < GP_OK)
		return result;
	result = gp_widget_set_value (widget, value);
	if (result >= GP_OK)
		result = gp_camera_set_single_config (camera, "pentaxdirectshutter",
			widget, context);
	gp_widget_free (widget);
	return result;
}

int
main (int argc, char **argv)
{
	Camera *camera = NULL;
	GPContext *context = NULL;
	unsigned int num = 0, den = 0;
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
	result = read_shutter (camera, context, &num, &den);
	if (result < GP_OK)
		goto out;
	printf ("stage=initial conditions_shutter=%u/%u\n", num, den);
	if ((num != 1U) || (den != 500U)) {
		result = GP_ERROR_BAD_PARAMETERS;
		goto out;
	}

	set_result = set_shutter (camera, context, "1/125");
	result = read_shutter (camera, context, &num, &den);
	if (result < GP_OK)
		goto out;
	printf ("stage=target set_result=%d conditions_shutter=%u/%u\n",
		set_result, num, den);
	changed = (num != 1U) || (den != 500U);
	if ((set_result < GP_OK) && !changed) {
		result = set_result;
		goto out;
	}
	if ((set_result < GP_OK) || (num != 1U) || (den != 125U))
		result = GP_ERROR;

	if (changed) {
		restore_result = set_shutter (camera, context, "1/500");
		if (read_shutter (camera, context, &num, &den) < GP_OK) {
			num = 0;
			den = 0;
		}
		printf ("stage=restore set_result=%d conditions_shutter=%u/%u\n",
			restore_result, num, den);
		if ((restore_result < GP_OK) || (num != 1U) || (den != 500U))
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
		fprintf (stderr, "shutter round trip failed: %s (%d); changed=%d; "
			"restore_result=%d\n", gp_result_as_string (result), result,
			changed, restore_result);
		return 1;
	}
	printf ("shutter_roundtrip=pass cleanup=ok\n");
	return 0;
}
