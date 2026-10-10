#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <gphoto2/gphoto2.h>
#include "pentax-bulb-lifecycle.h"
#include "ptp.h"
#include "ptp-private.h"
#include "samples.h"

#define MODEL "Pentax K-3 Mark III (MTP mode)"
#define CONFIRM_PREFIX "CAPTURE "

static int
read_conditions (Camera *camera, GPContext *context, unsigned int *state,
	unsigned int *mode, int *camera_timer, int *camera_idle)
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
		*camera_idle = value && strstr (value, "shooting=no") &&
			strstr (value, "processing=no") &&
			strstr (value, "task-changing=no") &&
			(*state == 0 || *state == 48);
		if (value)
			printf ("conditions=%s\n", value);
	}
	if (widget)
		gp_widget_free (widget);
	return result;
}

static int
safe_name (const char *name)
{
	if (!name || !name[0] || strchr (name, '/') || strstr (name, ".."))
		return 0;
	for (const unsigned char *p = (const unsigned char *)name; *p; p++)
		if (!isalnum (*p) && *p != '.' && *p != '_' && *p != '-')
			return 0;
	return 1;
}

static int
list_contains (CameraList *list, const char *name)
{
	for (int i = 0; i < gp_list_count (list); i++) {
		const char *item = NULL;
		if (gp_list_get_name (list, i, &item) >= GP_OK && item &&
		    !strcmp (item, name))
			return 1;
	}
	return 0;
}

static int
save_file (Camera *camera, GPContext *context, const char *output_dir,
	const char *name)
{
	CameraFile *file = NULL;
	const char *bytes = NULL;
	unsigned long size = 0, offset = 0;
	char path[1024];
	int fd, result;

	if (!safe_name (name) ||
	    snprintf (path, sizeof (path), "%s/%s", output_dir, name) >= (int)sizeof (path))
		return GP_ERROR_BAD_PARAMETERS;
	result = gp_file_new (&file);
	if (result >= GP_OK)
		result = gp_camera_file_get (camera, "/", name,
			GP_FILE_TYPE_NORMAL, file, context);
	if (result >= GP_OK)
		result = gp_file_get_data_and_size (file, &bytes, &size);
	if (result < GP_OK || !bytes || !size) {
		if (file)
			gp_file_unref (file);
		return result < GP_OK ? result : GP_ERROR_CORRUPTED_DATA;
	}
	fd = open (path, O_WRONLY | O_CREAT | O_EXCL, 0600);
	if (fd < 0) {
		gp_file_unref (file);
		return GP_ERROR_IO;
	}
	while (offset < size) {
		ssize_t written = write (fd, bytes + offset, size - offset);
		if (written <= 0) {
			close (fd);
			gp_file_unref (file);
			return GP_ERROR_IO;
		}
		offset += (unsigned long)written;
	}
	if (close (fd) < 0) {
		gp_file_unref (file);
		return GP_ERROR_IO;
	}
	printf ("saved_file=%s bytes=%lu\n", path, size);
	gp_file_unref (file);
	return GP_OK;
}

static volatile sig_atomic_t received_signal;

typedef struct {
	Camera *camera;
	GPContext *context;
	CameraWidget *bulb;
	CameraList **known;
	const char *output_dir;
	int shot;
} BulbCapture;

static void
handle_signal (int signal_number)
{
	received_signal = signal_number;
}

static int
install_signal_handlers (void)
{
	struct sigaction action;
	memset (&action, 0, sizeof (action));
	action.sa_handler = handle_signal;
	sigemptyset (&action.sa_mask);
	if (sigaction (SIGINT, &action, NULL) < 0 ||
	    sigaction (SIGTERM, &action, NULL) < 0)
		return GP_ERROR_IO;
	return GP_OK;
}

static int
check_cancel (void *opaque)
{
	(void)opaque;
	return received_signal ? GP_ERROR_CANCEL : GP_OK;
}

static int
set_bulb_edge (void *opaque, int start)
{
	BulbCapture *capture = opaque;
	int value = -1;
	int result = gp_widget_set_value (capture->bulb, &start);
	if (result >= GP_OK)
		result = gp_widget_get_value (capture->bulb, &value);
	if (result < GP_OK || value != start)
		return result < GP_OK ? result : GP_ERROR_BAD_PARAMETERS;
	printf ("shot=%d edge=%s\n", capture->shot, start ? "start" : "stop");
	result = gp_camera_set_single_config (capture->camera, "bulb",
		capture->bulb, capture->context);
	printf ("shot=%d %s_result=%d\n", capture->shot,
		start ? "start" : "stop", result);
	return result;
}

static int
wait_one_second (void *opaque)
{
	struct timespec remaining = {1, 0};
	(void)opaque;
	if (nanosleep (&remaining, NULL) == 0)
		return GP_OK;
	return errno == EINTR ? GP_ERROR_CANCEL : GP_ERROR_IO;
}

static int
collect_outputs (void *opaque)
{
	BulbCapture *capture = opaque;
	CameraList *after = NULL;
	int added = 0;
	int result = gp_list_new (&after);
	if (result >= GP_OK)
		result = gp_camera_folder_list_files (capture->camera, "/", after,
			capture->context);
	if (result < GP_OK)
		goto done;
	for (int i = 0; i < gp_list_count (after); i++) {
		const char *name = NULL;
		result = gp_list_get_name (after, i, &name);
		if (result < GP_OK || !name) {
			result = result < GP_OK ? result : GP_ERROR_CORRUPTED_DATA;
			goto done;
		}
		if (list_contains (*capture->known, name))
			continue;
		printf ("shot=%d published_file=%s\n", capture->shot, name);
		result = save_file (capture->camera, capture->context,
			capture->output_dir, name);
		if (result < GP_OK)
			goto done;
		added++;
	}
	if (!added) {
		fprintf (stderr, "shot=%d output_not_proven; do_not_retry\n",
			capture->shot);
		result = GP_ERROR_FILE_NOT_FOUND;
		goto done;
	}
	gp_list_free (*capture->known);
	*capture->known = after;
	after = NULL;

done:
	if (after)
		gp_list_free (after);
	return result;
}

int
main (int argc, char **argv)
{
	Camera *camera = NULL;
	GPContext *context = NULL;
	CameraAbilities abilities;
	CameraWidget *bulb = NULL;
	CameraList *known = NULL;
	unsigned int state = 0, mode = 0;
	int camera_timer = 0, camera_idle = 0;
	int count, result = GP_OK, initialized = 0, exit_result = GP_OK;
	int operator_intervention_required = 0;
	char confirmation[64], expected[64];
	PentaxBulbLifecycleState lifecycle_state = {0};

	if ((argc != 6) || strcmp (argv[1], "--execute") ||
	    strcmp (argv[2], MODEL) || !isatty (STDIN_FILENO) ||
	    sscanf (argv[5], "%d", &count) != 1 || count < 1 || count > 2) {
		fprintf (stderr, "usage: %s --execute \"%s\" usb:BUS,DEVICE OUTPUT_DIR 1|2\n",
			argv[0], MODEL);
		return 2;
	}
	if (access (argv[4], W_OK | X_OK) != 0) {
		fprintf (stderr, "output directory unavailable: %s (%s)\n",
			argv[4], strerror (errno));
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
	printf ("admission_state recovery=%d output_pending=%d\n",
		camera->pl->params.pentax.recovery_required,
		camera->pl->params.pentax.capture_output_pending);
	result = read_conditions (camera, context, &state, &mode,
		&camera_timer, &camera_idle);
	if (result < GP_OK || !camera_idle || mode != 9 || camera_timer) {
		fprintf (stderr, "preflight=refused state=%u mode=%u timer=%d idle=%d\n",
			state, mode, camera_timer, camera_idle);
		result = GP_ERROR_CAMERA_BUSY;
		goto done;
	}
	result = gp_list_new (&known);
	if (result >= GP_OK)
		result = gp_camera_folder_list_files (camera, "/", known, context);
	if (result < GP_OK) {
		fprintf (stderr, "baseline_file_list=failed result=%d; refusing shutter action\n",
			result);
		goto done;
	}
	result = gp_camera_get_single_config (camera, "bulb", &bulb, context);
	if (result < GP_OK) {
		fprintf (stderr, "bulb_action=unavailable result=%d\n", result);
		goto done;
	}
	snprintf (expected, sizeof (expected), CONFIRM_PREFIX "%d\n", count);
	printf ("This will make %d one-second B-mode capture(s), stop each through "
		"the Pentax Bulb action, and save every new DNG/JPEG before exit. "
		"Type %s to continue: ", count, expected);
	fflush (stdout);
	if (!fgets (confirmation, sizeof (confirmation), stdin) ||
	    strcmp (confirmation, expected)) {
		result = GP_ERROR_CANCEL;
		goto done;
	}

	result = install_signal_handlers ();
	if (result < GP_OK)
		goto done;
	for (int shot = 1; shot <= count; shot++) {
		BulbCapture capture = {
			.camera = camera,
			.context = context,
			.bulb = bulb,
			.known = &known,
			.output_dir = argv[4],
			.shot = shot,
		};
		const PentaxBulbLifecycleOps ops = {
			.opaque = &capture,
			.set_edge = set_bulb_edge,
			.check_cancel = check_cancel,
			.wait = wait_one_second,
			.collect = collect_outputs,
		};
		if (received_signal) {
			result = GP_ERROR_CANCEL;
			goto done;
		}
		result = pentax_bulb_run_lifecycle (&ops, &lifecycle_state);
		operator_intervention_required =
			lifecycle_state.operator_intervention_required;
		if (lifecycle_state.explicit_stop_attempted &&
		    lifecycle_state.explicit_stop_result < GP_OK)
			fprintf (stderr, "shot=%d explicit_stop_unconfirmed=%d\n", shot,
				lifecycle_state.explicit_stop_result);
		if (lifecycle_state.cleanup_stop_attempted)
			fprintf (stderr, "shot=%d cleanup_stop_result=%d shutter_open=%d\n",
				shot, lifecycle_state.cleanup_stop_result,
				lifecycle_state.shutter_open);
		if (lifecycle_state.collection_attempted &&
		    lifecycle_state.collection_result < GP_OK)
			fprintf (stderr, "shot=%d output_collection_error=%d; original_result=%d\n",
				shot, lifecycle_state.collection_result, result);
		if (operator_intervention_required) {
			if (lifecycle_state.start_confirmed)
				fprintf (stderr, "shot=%d OPERATOR_INTERVENTION_REQUIRED "
					"shutter_open=%d; do not issue another shutter action\n",
					shot, lifecycle_state.shutter_open);
			else
				fprintf (stderr, "shot=%d OPERATOR_INTERVENTION_REQUIRED "
					"start_unconfirmed=1 shutter_state=unknown; do not issue "
					"another shutter action\n", shot);
			goto done;
		}
		if (result < GP_OK)
			goto done;
		result = read_conditions (camera, context, &state, &mode,
			&camera_timer, &camera_idle);
		if (result < GP_OK || !camera_idle || mode != 9 || camera_timer) {
			fprintf (stderr, "shot=%d postflight=not_idle state=%u mode=%u "
				"timer=%d idle=%d\n", shot, state, mode,
				camera_timer, camera_idle);
			result = GP_ERROR_CAMERA_BUSY;
			goto done;
		}
	}
	result = read_conditions (camera, context, &state, &mode,
		&camera_timer, &camera_idle);
	if (result < GP_OK || !camera_idle || mode != 9) {
		fprintf (stderr, "postflight=not_idle state=%u mode=%u idle=%d\n",
			state, mode, camera_idle);
		result = GP_ERROR_CAMERA_BUSY;
		goto done;
	}
	printf ("bulb_edge_test=pass shots=%d output_dir=%s\n", count, argv[4]);

done:
	if (initialized && operator_intervention_required) {
		fprintf (stderr, "camera_exit=skipped; shutter/session state requires operator inspection\n");
	} else if (initialized) {
		exit_result = gp_camera_exit (camera, context);
		if (result >= GP_OK && exit_result < GP_OK)
			result = exit_result;
	}
	if (bulb)
		gp_widget_free (bulb);
	if (known)
		gp_list_free (known);
	if (camera)
		gp_camera_unref (camera);
	if (context)
		gp_context_unref (context);
	if (result < GP_OK) {
		fprintf (stderr, "test_result=fail result=%d cleanup=%d; inspect outputs and do not retry blindly\n",
			result, exit_result);
		return 1;
	}
	return 0;
}
