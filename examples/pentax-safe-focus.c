/* One-command Pentax K-3 III minimum focus-drive hardware probe. */
#include "config.h"

#include <stdio.h>
#include <string.h>

#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-widget.h>

#include "samples.h"

#define PENTAX_MODEL "Pentax:K-3 Mark III (MTP mode)"

int
main (int argc, char **argv)
{
	Camera *camera = NULL;
	CameraWidget *widget = NULL;
	GPContext *context = NULL;
	const char *action;
	const char *stage = "create-context";
	int initialized = 0, enabled = 1;
	int result = GP_OK, exit_result;

	if ((argc != 3) || (strcmp (argv[2], "init") &&
	    strcmp (argv[2], "near") && strcmp (argv[2], "far"))) {
		fprintf (stderr, "usage: %s usb:BUS,DEVICE init|near|far\n", argv[0]);
		return 2;
	}
	action = !strcmp (argv[2], "near") ?
		"manualfocusdrivenear" : "manualfocusdrivefar";
	context = sample_create_context ();
	if (!context)
		return 1;
	stage = "open-explicit-camera";
	result = sample_open_camera (&camera, PENTAX_MODEL, argv[1], context);
	if (result < GP_OK)
		goto out;
	stage = "camera-init";
	result = gp_camera_init (camera, context);
	if (result < GP_OK)
		goto out;
	initialized = 1;
	if (!strcmp (argv[2], "init")) {
		stage = "init-only-complete";
		goto out;
	}
	stage = "get-focus-action";
	result = gp_camera_get_single_config (camera, action, &widget, context);
	if (result < GP_OK)
		goto out;
	result = gp_widget_set_value (widget, &enabled);
	if (result < GP_OK)
		goto out;
	stage = "send-minimum-focus";
	result = gp_camera_set_single_config (camera, action, widget, context);

out:
	if (widget)
		gp_widget_free (widget);
	if (initialized) {
		exit_result = gp_camera_exit (camera, context);
		if ((result >= GP_OK) && (exit_result < GP_OK))
			result = exit_result;
	}
	if (camera)
		gp_camera_unref (camera);
	gp_context_unref (context);
	if (result < GP_OK) {
		fprintf (stderr, "focus_%s=failed stage=%s error=%s (%d) cleanup=attempted\n",
			argv[2], stage, gp_result_as_string (result), result);
		return 1;
	}
	printf ("focus_%s=command-accepted cleanup=ok retries=0\n", argv[2]);
	return 0;
}
