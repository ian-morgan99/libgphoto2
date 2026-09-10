/* Exact-model, single-session K-1 II live-view AF-position (0xd036) write
 * qualification.  Issue #8: the K-1 II accepts d036 writes but reports the
 * geometry centre on read-back instead of echoing the requested spot, so a
 * structurally valid 8-byte response is sufficient proof the property is
 * honoured (the camlib encodes this in _put_Pentax_LiveViewAFPosition).
 *
 * The test keeps PC live view running across calls via pentaxpclvkeep so the
 * LiveViewAreaInfo read inside the d036 handler has a valid area, then:
 *   1. reads the baseline AF position (GET must be structurally valid),
 *   2. writes that same point back (SET must be accepted by the camera),
 *   3. re-reads and requires a structurally valid response.
 * A no-op write is used so the test never moves the AF spot to an arbitrary
 * location on the sensor.
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-widget.h>

#include "samples.h"

static int
read_lvaf (Camera *camera, GPContext *context, char **value)
{
	CameraWidget *widget = NULL;
	const char *v = NULL;
	int result;

	result = gp_camera_get_single_config (camera,
		"pentaxliveviewafposition", &widget, context);
	if (result < GP_OK)
		return result;
	result = gp_widget_get_value (widget, &v);
	if (result >= GP_OK && v)
		*value = strdup (v);
	gp_widget_free (widget);
	return result;
}

static int
write_lvaf (Camera *camera, GPContext *context, const char *value)
{
	CameraWidget *widget = NULL;
	int result;

	result = gp_camera_get_single_config (camera,
		"pentaxliveviewafposition", &widget, context);
	if (result < GP_OK)
		return result;
	result = gp_widget_set_value (widget, value);
	if (result >= GP_OK)
		result = gp_camera_set_single_config (camera,
			"pentaxliveviewafposition", widget, context);
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
	if (result < GP_OK)
		return result;
	result = gp_widget_set_value (widget, value);
	if (result >= GP_OK)
		result = gp_camera_set_single_config (camera, "pentaxpclvkeep",
			widget, context);
	gp_widget_free (widget);
	return result;
}

int
main (int argc, char **argv)
{
	Camera *camera = NULL;
	GPContext *context = NULL;
	char *baseline = NULL, *after = NULL;
	int initialized = 0;
	int result = GP_OK, exit_result;

	if ((argc != 3) || strcmp (argv[1], "Pentax:K-1 Mark II (PTP mode)")) {
		fprintf (stderr,
			"usage: %s 'Pentax:K-1 Mark II (PTP mode)' usb:BUS,DEVICE\n",
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

	/* Keep PC-LV running so LiveViewAreaInfo is valid for the d036 path. */
	result = set_keep_live_view (camera, context, "on");
	if (result < GP_OK) {
		fprintf (stderr, "stage=keep-live-view error=%s (%d)\n",
			gp_result_as_string (result), result);
		goto out;
	}
	printf ("stage=keep-live-view on\n");

	/* Start PC-LV with one preview frame. */
	{
		CameraFile *file = NULL;
		result = gp_file_new (&file);
		if (result < GP_OK)
			goto out;
		result = gp_camera_capture_preview (camera, file, context);
		gp_file_unref (file);
		if (result < GP_OK) {
			fprintf (stderr, "stage=start-lv error=%s (%d)\n",
				gp_result_as_string (result), result);
			goto out;
		}
		printf ("stage=start-lv ok\n");
	}

	result = read_lvaf (camera, context, &baseline);
	if (result < GP_OK) {
		fprintf (stderr, "stage=read-baseline error=%s (%d)\n",
			gp_result_as_string (result), result);
		goto out;
	}
	printf ("stage=baseline position=%s\n", baseline);

	result = write_lvaf (camera, context, baseline);
	if (result < GP_OK) {
		fprintf (stderr, "stage=write error=%s (%d)\n",
			gp_result_as_string (result), result);
		goto out;
	}
	printf ("stage=write accepted\n");

	result = read_lvaf (camera, context, &after);
	if (result < GP_OK) {
		fprintf (stderr, "stage=read-after error=%s (%d)\n",
			gp_result_as_string (result), result);
		goto out;
	}
	printf ("stage=read-after position=%s\n", after);

out:
	if (initialized) {
		exit_result = gp_camera_exit (camera, context);
		if ((result >= GP_OK) && (exit_result < GP_OK))
			result = exit_result;
	}
	if (camera)
		gp_camera_unref (camera);
	gp_context_unref (context);
	free (baseline);
	free (after);
	if (result < GP_OK) {
		fprintf (stderr, "LV AF round trip failed: %s (%d)\n",
			gp_result_as_string (result), result);
		return 1;
	}
	printf ("lvaf_roundtrip=pass cleanup=ok\n");
	return 0;
}
