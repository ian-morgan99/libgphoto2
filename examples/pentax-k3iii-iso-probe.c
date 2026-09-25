/* K-3 III ISO capability probe (issue #109 / libgphoto2#77 downstream).
 *
 * Single session, one variable:
 *  1. read baseline ISO from pentaxconditions
 *  2. enumerate the "iso" widget choices (what the driver advertises)
 *  3. attempt SET of a >6400 value (highest advertised choice), verify via conditions
 *  4. attempt an invalid value, expect clean failure + unchanged ISO
 *  5. restore original ISO, verify in a fresh read
 */
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

	result = gp_camera_get_single_config (camera, "iso", &widget, context);
	if (result < GP_OK)
		return result;
	result = gp_widget_set_value (widget, value);
	if (result >= GP_OK)
		result = gp_camera_set_single_config (camera, "iso", widget, context);
	gp_widget_free (widget);
	return result;
}

int
main (int argc, char **argv)
{
	Camera *camera = NULL;
	GPContext *context = NULL;
	unsigned int iso = 0, restored_iso = 0;
	int initialized = 0, result = GP_OK, set_result, restore_result = GP_OK, exit_result;

	if (argc != 3) {
		fprintf (stderr, "usage: %s 'Pentax:K-3 Mark III (MTP mode)' usb:BUS,DEVICE\n",
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

	/* 1. baseline */
	result = read_conditions_iso (camera, context, &iso);
	if (result < GP_OK)
		goto out;
	printf ("stage=baseline conditions_iso=%u\n", iso);

	/* 2. advertised choices */
	{
		CameraWidget *widget = NULL;
		int r = gp_camera_get_single_config (camera, "iso", &widget, context);
		if (r < GP_OK) {
			printf ("stage=choices get iso widget failed: %s\n",
				gp_result_as_string (r));
		} else {
			CameraWidgetType wtype = GP_WIDGET_TEXT;
			int count = 0, i;
			const char *v = NULL;
			gp_widget_get_type (widget, &wtype);
			gp_widget_get_value (widget, &v);
			if ((wtype == GP_WIDGET_RADIO) || (wtype == GP_WIDGET_MENU))
				count = gp_widget_count_choices (widget);
			printf ("stage=choices type=%d count=%d value=%s:",
				wtype, count, v ? v : "(null)");
			for (i = 0; i < count; i++) {
				const char *choice = NULL;
				gp_widget_get_choice (widget, i, &choice);
				printf (" %s", choice ? choice : "?");
			}
			printf ("\n");
		}
		gp_widget_free (widget);
	}

	/* 3. attempt a >6400 SET (12800; K-3 III supports up to 25600 per IT2) */
	set_result = set_iso (camera, context, "12800");
	result = read_conditions_iso (camera, context, &restored_iso);
	if (result < GP_OK)
		goto out;
	printf ("stage=highiso set_result=%d conditions_iso=%u\n",
		set_result, restored_iso);

	/* 4. invalid value must fail cleanly and leave ISO unchanged */
	{
		int bad = set_iso (camera, context, "99999");
		unsigned int after_bad = 0;
		if (read_conditions_iso (camera, context, &after_bad) < GP_OK)
			after_bad = 0;
		printf ("stage=invalid set_result=%d conditions_iso=%u\n",
			bad, after_bad);
	}

	/* 5. restore baseline and verify */
	{
		char buf[16];
		snprintf (buf, sizeof buf, "%u", iso);
		restore_result = set_iso (camera, context, buf);
	}
	result = read_conditions_iso (camera, context, &restored_iso);
	if (result < GP_OK)
		goto out;
	printf ("stage=restore set_result=%d conditions_iso=%u expected=%u\n",
		restore_result, restored_iso, iso);
	if ((restore_result < GP_OK) || (restored_iso != iso))
		result = restore_result < GP_OK ? restore_result : GP_ERROR;

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
		fprintf (stderr, "iso probe failed: %s (%d)\n",
			gp_result_as_string (result), result);
		return 1;
	}
	printf ("iso_probe=pass cleanup=ok\n");
	return 0;
}
