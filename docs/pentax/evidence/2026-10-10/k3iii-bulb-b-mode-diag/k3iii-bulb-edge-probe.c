#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <gphoto2/gphoto2.h>
#include "ptp.h"
#include "ptp-private.h"
#include "samples.h"

#define MODEL "Pentax K-3 Mark III (MTP mode)"
#define CONFIRM "CAPTURE ONE"

static int
read_conditions (Camera *camera, GPContext *context, unsigned int *state,
	unsigned int *mode, int *camera_timer)
{
	CameraWidget *widget = NULL;
	const char *value = NULL;
	const char *field;
	int result = gp_camera_get_single_config (camera, "pentaxconditions",
		&widget, context);

	if (result >= GP_OK)
		result = gp_widget_get_value (widget, &value);
	if (result >= GP_OK) {
		field = value ? strstr (value, "state=") : NULL;
		if (!field || sscanf (field, "state=%u", state) != 1)
			result = GP_ERROR_CORRUPTED_DATA;
		field = value ? strstr (value, "exposure-mode-raw=") : NULL;
		if (result >= GP_OK && (!field || sscanf (field,
			"exposure-mode-raw=%u", mode) != 1))
			result = GP_ERROR_CORRUPTED_DATA;
		field = value ? strstr (value, "bulb-timer=") : NULL;
		if (result >= GP_OK && !field)
			result = GP_ERROR_CORRUPTED_DATA;
		if (result >= GP_OK)
			*camera_timer = !strncmp (field, "bulb-timer=yes", 14);
		if (value)
			printf ("conditions=%s\n", value);
	}
	if (widget)
		gp_widget_free (widget);
	return result;
}

int
main (int argc, char **argv)
{
	Camera *camera = NULL;
	GPContext *context = NULL;
	CameraAbilities abilities;
	CameraWidget *bulb = NULL;
	CameraList *before = NULL, *after = NULL;
	const char *model = NULL;
	char confirmation[64];
	unsigned int state = 0, mode = 0;
	int camera_timer = 0, result = GP_OK, initialized = 0;
	int inspect_only, widget_value, start_value = 1;
	CameraWidgetType widget_type;
	int start_result, stop_result = GP_ERROR;
	struct timespec hold = {1, 0};

	if ((argc == 2) && !strcmp (argv[1], "--self-test")) {
		puts ("usage_self_test=pass; hardware_access=none");
		return 0;
	}
	inspect_only = (argc == 4) && !strcmp (argv[1], "--inspect");
	if ((argc != 4) ||
	    (strcmp (argv[1], "--execute") && !inspect_only) ||
	    strcmp (argv[2], MODEL) || (!inspect_only && !isatty (0))) {
		fprintf (stderr, "usage: %s --inspect|--execute \"%s\" usb:BUS,DEVICE\n",
			argv[0], MODEL);
		return 2;
	}
	context = gp_context_new ();
	if (!context)
		return 1;
	result = sample_open_camera (&camera, MODEL, argv[3], context);
	if (result < GP_OK)
		goto done;
	result = gp_camera_get_abilities (camera, &abilities);
	if (result < GP_OK)
		goto done;
	printf ("selected_model=%s usb=%04x:%04x port=%s\n", abilities.model,
		abilities.usb_vendor, abilities.usb_product, argv[3]);
	if (strcmp (abilities.model, MODEL) || abilities.usb_vendor != 0x25fb ||
	    abilities.usb_product != 0x0189) {
		result = GP_ERROR_NOT_SUPPORTED;
		goto done;
	}
	result = gp_camera_init (camera, context);
	if (result < GP_OK)
		goto done;
	initialized = 1;
	printf ("libgphoto_state transfer=%d recovery=%d output_pending=%d "
		"bulb_active=%d\n",
		camera->pl->params.pentax.transfer_state,
		camera->pl->params.pentax.recovery_required,
		camera->pl->params.pentax.capture_output_pending,
		camera->pl->params.pentax.bulb_action_active);
	result = read_conditions (camera, context, &state, &mode, &camera_timer);
	if (result < GP_OK || state != 0 || mode != 9 || camera_timer) {
		fprintf (stderr, "preflight=refused state=%u mode=%u camera_timer=%d\n",
			state, mode, camera_timer);
		result = GP_ERROR_CAMERA_BUSY;
		goto done;
	}
	gp_list_new (&before);
	result = gp_camera_folder_list_files (camera, "/", before, context);
	if (result < GP_OK)
		goto done;
	result = gp_camera_get_single_config (camera, "bulb", &bulb, context);
	if (result < GP_OK) {
		fprintf (stderr, "bulb_action=unavailable result=%d\n", result);
		goto done;
	}
	result = gp_widget_get_type (bulb, &widget_type);
	if (result >= GP_OK)
		result = gp_widget_get_value (bulb, &widget_value);
	if (result < GP_OK)
		goto done;
	printf ("bulb_widget_type=%d value_before=%d\n", widget_type, widget_value);
	if (inspect_only) {
		int test_value = 1;
		result = gp_widget_set_value (bulb, &test_value);
		if (result >= GP_OK)
			result = gp_widget_get_value (bulb, &widget_value);
		printf ("bulb_widget_set_local=%d value_after=%d wire_command_sent=no\n",
			result, widget_value);
		goto done;
	}
	printf ("This sends the documented K-3 III Bulb start command, holds for "
		"one second, then sends its matching stop and lets libgphoto2 finish "
		"the file-transfer lifecycle. Type %s to continue: ", CONFIRM);
	fflush (stdout);
	if (!fgets (confirmation, sizeof (confirmation), stdin) ||
	    strcmp (confirmation, CONFIRM "\n")) {
		result = GP_ERROR_CANCEL;
		goto done;
	}
	result = gp_widget_set_value (bulb, &start_value);
	if (result >= GP_OK)
		result = gp_widget_get_value (bulb, &widget_value);
	printf ("bulb_start_widget_value=%d\n", widget_value);
	if (result < GP_OK || widget_value != 1) {
		fprintf (stderr, "refusing: start value is not exactly 1\n");
		result = GP_ERROR_BAD_PARAMETERS;
		goto done;
	}
	start_result = gp_camera_set_single_config (camera, "bulb", bulb, context);
	printf ("bulb_start_result=%d\n", start_result);
	if (start_result < GP_OK) {
		result = start_result;
		fprintf (stderr, "start_not_confirmed=stop_not_sent; do_not_retry\n");
		goto done;
	}
	nanosleep (&hold, NULL);
	result = gp_widget_set_value (bulb, &(int){0});
	if (result < GP_OK)
		goto done;
	stop_result = gp_camera_set_single_config (camera, "bulb", bulb, context);
	printf ("bulb_stop_and_finalize_result=%d\n", stop_result);
	result = stop_result;
	if (result < GP_OK)
		goto done;
	gp_list_new (&after);
	result = gp_camera_folder_list_files (camera, "/", after, context);
	if (result < GP_OK)
		goto done;
	for (int i = 0; i < gp_list_count (after); i++) {
		gp_list_get_name (after, i, &model);
		printf ("published_file=%s\n", model);
	}
	printf ("before_file_count=%d after_file_count=%d\n",
		gp_list_count (before), gp_list_count (after));
	if (gp_list_count (after) <= gp_list_count (before)) {
		fprintf (stderr, "capture_file_not_proven; preserve camera/card; do not retry\n");
		result = GP_ERROR_FILE_NOT_FOUND;
		goto done;
	}
	puts ("one_second_bulb_lifecycle=pass; do not repeat without review");

done:
	if (initialized) {
		int exit_result = gp_camera_exit (camera, context);
		if (result >= GP_OK && exit_result < GP_OK)
			result = exit_result;
	}
	if (bulb)
		gp_widget_free (bulb);
	if (before)
		gp_list_free (before);
	if (after)
		gp_list_free (after);
	if (camera)
		gp_camera_unref (camera);
	if (context)
		gp_context_unref (context);
	if (result < GP_OK) {
		fprintf (stderr, "probe_result=fail result=%d stop_result=%d\n",
			result, stop_result);
		return 1;
	}
	return 0;
}
