/* Exact-model Pentax shutter round trip with live-baseline restoration. */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-widget.h>
#include "samples.h"

static int
parse_shutter (const char *value, unsigned int *num, unsigned int *den)
{
	return ((sscanf (value, "%u/%u", num, den) == 2) && *den) ? GP_OK :
		GP_ERROR_BAD_PARAMETERS;
}

static int
read_conditions (Camera *camera, GPContext *context, unsigned int *num,
	unsigned int *den)
{
	CameraWidget *widget = NULL;
	const char *value = NULL, *start;
	int result = gp_camera_get_single_config (camera, "pentaxconditions",
		&widget, context);

	if (result >= GP_OK)
		result = gp_widget_get_value (widget, &value);
	if (result >= GP_OK) {
		start = strstr (value, "bulb-seconds=");
		if (!start || (sscanf (start, "bulb-seconds=%u/%u", num, den) != 2) ||
		    !*den)
			result = GP_ERROR_CORRUPTED_DATA;
	}
	if (widget)
		gp_widget_free (widget);
	return result;
}

static int
read_values (Camera *camera, GPContext *context, char **baseline, char **target)
{
	CameraWidget *widget = NULL;
	const char *value = NULL, *choice = NULL;
	int count, i, result = gp_camera_get_single_config (camera,
		"pentaxdirectshutter", &widget, context);

	if (result >= GP_OK)
		result = gp_widget_get_value (widget, &value);
	if (result < GP_OK)
		goto done;
	*baseline = strdup (value);
	if (!*baseline) {
		result = GP_ERROR_NO_MEMORY;
		goto done;
	}
	count = gp_widget_count_choices (widget);
	if (count < GP_OK) {
		result = count;
		goto done;
	}
	for (i = 0; i < count; i++) {
		unsigned int num, den;

		result = gp_widget_get_choice (widget, i, &choice);
		if (result < GP_OK)
			goto done;
		/* Exclude Bulb and exposures longer than 1/30 second. */
		if (!strcmp (choice, *baseline) ||
		    (parse_shutter (choice, &num, &den) < GP_OK) ||
		    ((unsigned long)num * 30UL > den))
			continue;
		*target = strdup (choice);
		result = *target ? GP_OK : GP_ERROR_NO_MEMORY;
		goto done;
	}
	result = GP_ERROR_NOT_SUPPORTED;
done:
	if (widget)
		gp_widget_free (widget);
	return result;
}

static int
set_shutter (Camera *camera, GPContext *context, const char *value)
{
	CameraWidget *widget = NULL;
	int result = gp_camera_get_single_config (camera, "pentaxdirectshutter",
		&widget, context);

	if (result >= GP_OK)
		result = gp_widget_set_value (widget, value);
	if (result >= GP_OK)
		result = gp_camera_set_single_config (camera, "pentaxdirectshutter",
			widget, context);
	if (widget)
		gp_widget_free (widget);
	return result;
}

static int
matches (Camera *camera, GPContext *context, const char *expected,
	unsigned int *actual_num, unsigned int *actual_den)
{
	unsigned int num, den;
	int result = parse_shutter (expected, &num, &den);

	if (result >= GP_OK)
		result = read_conditions (camera, context, actual_num, actual_den);
	if (result < GP_OK)
		return result;
	return ((unsigned long)num * *actual_den ==
		(unsigned long)*actual_num * den) ? GP_OK : GP_ERROR;
}

int
main (int argc, char **argv)
{
	Camera *camera = NULL;
	GPContext *context = NULL;
	CameraAbilities abilities;
	char *baseline = NULL, *target = NULL;
	unsigned int num = 0, den = 0;
	int initialized = 0, restore_required = 0;
	int result = GP_OK, restore_result = GP_OK, exit_result;

	if ((argc != 3) ||
	    (strcmp (argv[1], "Pentax K-1 Mark II (PTP mode)") &&
	     strcmp (argv[1], "Pentax K-3 Mark III (MTP mode)"))) {
		fprintf (stderr, "usage: %s MODEL usb:BUS,DEVICE\n", argv[0]);
		return 2;
	}
	context = sample_create_context ();
	if (!context)
		return 1;
	result = sample_open_camera (&camera, argv[1], argv[2], context);
	if (result < GP_OK)
		goto out;
	result = gp_camera_get_abilities (camera, &abilities);
	if (result < GP_OK)
		goto out;
	printf ("selected_model=%s usb=%04x:%04x port=%s\n", abilities.model,
		abilities.usb_vendor, abilities.usb_product, argv[2]);
	result = gp_camera_init (camera, context);
	if (result < GP_OK)
		goto out;
	initialized = 1;
	result = read_values (camera, context, &baseline, &target);
	if (result < GP_OK)
		goto out;
	result = matches (camera, context, baseline, &num, &den);
	printf ("stage=initial widget=%s conditions_shutter=%u/%u\n", baseline,
		num, den);
	if (result < GP_OK)
		goto out;
	restore_required = 1; /* SET may take effect even if its response is lost. */
	result = set_shutter (camera, context, target);
	if ((matches (camera, context, target, &num, &den) < GP_OK) &&
	    (result >= GP_OK))
		result = GP_ERROR;
	printf ("stage=target widget=%s set_result=%d conditions_shutter=%u/%u\n",
		target, result, num, den);
out:
	if (restore_required && camera && baseline) {
		restore_result = set_shutter (camera, context, baseline);
		if ((restore_result >= GP_OK) &&
		    (matches (camera, context, baseline, &num, &den) < GP_OK))
			restore_result = GP_ERROR;
		printf ("stage=restore widget=%s set_result=%d conditions_shutter=%u/%u\n",
			baseline, restore_result, num, den);
		if (restore_result < GP_OK)
			result = restore_result;
	}
	if (initialized) {
		exit_result = gp_camera_exit (camera, context);
		if ((result >= GP_OK) && (exit_result < GP_OK))
			result = exit_result;
	}
	if (camera)
		gp_camera_unref (camera);
	gp_context_unref (context);
	free (target);
	free (baseline);
	if (result < GP_OK) {
		fprintf (stderr, "shutter round trip failed: %s (%d); restore=%d\n",
			gp_result_as_string (result), result, restore_result);
		return 1;
	}
	printf ("shutter_roundtrip=pass cleanup=ok\n");
	return 0;
}
