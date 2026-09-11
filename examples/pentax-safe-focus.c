/* One-command exact-model Pentax minimum focus-drive hardware probe. */
#include "config.h"

#include <stdio.h>
#include <string.h>

#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-file.h>
#include <gphoto2/gphoto2-widget.h>

#include "samples.h"

static int
set_named_value (Camera *camera, GPContext *context, const char *name,
	const void *value)
{
	CameraWidget *widget = NULL;
	int result;

	result = gp_camera_get_single_config (camera, name, &widget, context);
	if (result >= GP_OK)
		result = gp_widget_set_value (widget, value);
	if (result >= GP_OK)
		result = gp_camera_set_single_config (camera, name, widget, context);
	if (widget)
		gp_widget_free (widget);
	return result;
}

static int
capture_valid_preview (Camera *camera, GPContext *context, unsigned long *size)
{
	CameraFile *file = NULL;
	const char *data = NULL, *mime = NULL;
	int result;

	*size = 0;
	result = gp_file_new (&file);
	if (result >= GP_OK)
		result = gp_camera_capture_preview (camera, file, context);
	if (result >= GP_OK)
		result = gp_file_get_data_and_size (file, &data, size);
	if (result >= GP_OK)
		result = gp_file_get_mime_type (file, &mime);
	if ((result >= GP_OK) &&
	    (!data || *size < 4 || !mime || strcmp (mime, GP_MIME_JPEG) ||
	     (unsigned char)data[0] != 0xff || (unsigned char)data[1] != 0xd8 ||
	     (unsigned char)data[*size - 2] != 0xff ||
	     (unsigned char)data[*size - 1] != 0xd9))
		result = GP_ERROR_CORRUPTED_DATA;
	if (file)
		gp_file_unref (file);
	return result;
}

int
main (int argc, char **argv)
{
	Camera *camera = NULL;
	CameraWidget *widget = NULL;
	GPContext *context = NULL;
	const char *action;
	const char *stage = "create-context";
	int initialized = 0, enabled = 1, live_view_requested = 0;
	int result = GP_OK, exit_result, cleanup_result = GP_OK;

	if ((argc != 4) || (strcmp (argv[3], "init") &&
	    strcmp (argv[3], "near") && strcmp (argv[3], "far") &&
	    strcmp (argv[3], "near-lv") && strcmp (argv[3], "far-lv"))) {
		fprintf (stderr,
			"usage: %s MODEL usb:BUS,DEVICE init|near|far|near-lv|far-lv\n",
			argv[0]);
		return 2;
	}
	action = (!strcmp (argv[3], "near") || !strcmp (argv[3], "near-lv")) ?
		"manualfocusdrivenear" : "manualfocusdrivefar";
	context = sample_create_context ();
	if (!context)
		return 1;
	stage = "open-explicit-camera";
	result = sample_open_camera (&camera, argv[1], argv[2], context);
	if (result < GP_OK)
		goto out;
	{
		CameraAbilities abilities;
		result = gp_camera_get_abilities (camera, &abilities);
		if (result < GP_OK)
			goto out;
		printf ("selected_model=%s usb=%04x:%04x port=%s\n",
			abilities.model, abilities.usb_vendor, abilities.usb_product,
			argv[2]);
	}
	stage = "camera-init";
	result = gp_camera_init (camera, context);
	if (result < GP_OK)
		goto out;
	initialized = 1;
	if (!strcmp (argv[3], "init")) {
		stage = "init-only-complete";
		goto out;
	}
	if (strstr (argv[3], "-lv")) {
		unsigned long size = 0;

		stage = "enable-keep-live-view";
		result = set_named_value (camera, context, "pentaxpclvkeep", &enabled);
		if (result < GP_OK)
			goto out;
		live_view_requested = 1;
		stage = "start-live-view";
		result = capture_valid_preview (camera, context, &size);
		if (result < GP_OK)
			goto out;
		printf ("live_view=ready bytes=%lu\n", size);
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
	if ((result >= GP_OK) && live_view_requested) {
		unsigned long size = 0;

		stage = "post-focus-live-view";
		result = capture_valid_preview (camera, context, &size);
		if (result >= GP_OK)
			printf ("post_focus_live_view=ready bytes=%lu\n", size);
	}

out:
	if (widget)
		gp_widget_free (widget);
	if (live_view_requested && camera) {
		int disabled = 0;
		int off_result;

		off_result = set_named_value (camera, context, "pentaxpclvkeep", &disabled);
		if (off_result < GP_OK) {
			fprintf (stderr, "cleanup=keep-live-view-disable-failed error=%s (%d)\n",
				gp_result_as_string (off_result), off_result);
			cleanup_result = off_result;
		}
	}
	if (initialized) {
		exit_result = gp_camera_exit (camera, context);
		if ((result >= GP_OK) && (exit_result < GP_OK))
			result = exit_result;
	}
	if ((result >= GP_OK) && (cleanup_result < GP_OK))
		result = cleanup_result;
	if (camera)
		gp_camera_unref (camera);
	gp_context_unref (context);
	if (result < GP_OK) {
		fprintf (stderr, "focus_%s=failed stage=%s error=%s (%d) cleanup=attempted\n",
			argv[3], stage, gp_result_as_string (result), result);
		return 1;
	}
	printf ("focus_%s=command-accepted cleanup=ok retries=0\n", argv[3]);
	return 0;
}
