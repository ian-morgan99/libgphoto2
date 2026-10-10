#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gphoto2/gphoto2.h>

#include "ptp.h"
#include "ptp-private.h"
#include "pentax-utils.h"
#include "pentax-bulb-timed-probe-guard.h"
#include "../examples/samples.h"

#define EXPECTED_MODEL "Pentax K-3 Mark III (MTP mode)"
#define CONFIRMATION "CAPTURE ONE"

static const char *
guard_result_name (PentaxBulbProbeGuardResult result)
{
	switch (result) {
	case PENTAX_BULB_PROBE_SAFE: return "safe";
	case PENTAX_BULB_PROBE_WRONG_CAMERA: return "wrong-camera";
	case PENTAX_BULB_PROBE_NOT_READY: return "camera-not-ready";
	case PENTAX_BULB_PROBE_BUSY: return "camera-busy";
	case PENTAX_BULB_PROBE_WRONG_MODE: return "not-camera-timed-B-mode";
	case PENTAX_BULB_PROBE_TIMER_UNAVAILABLE: return "one-second-timer-unavailable";
	case PENTAX_BULB_PROBE_TIMER_TOO_LONG: return "timer-exceeds-one-second";
	}
	return "unknown";
}

static int
read_conditions (Camera *camera, PentaxConditions *conditions)
{
	unsigned char *data = NULL;
	unsigned int size = 0;
	uint16_t ptp_result;
	int result;

	ptp_result = ptp_pentax_get_all_conditions (&camera->pl->params,
		&data, &size);
	if (ptp_result != PTP_RC_OK) {
		fprintf (stderr, "conditions_read=failed ptp=0x%04x\n", ptp_result);
		free (data);
		return GP_ERROR;
	}
	result = pentax_parse_conditions (data, size, conditions);
	free (data);
	if (result < GP_OK)
		fprintf (stderr, "conditions_parse=failed result=%d size=%u\n",
			result, size);
	return result;
}

static int
conditions_idle (const PentaxConditions *conditions)
{
	return conditions && conditions->activity_flags == 0 &&
		conditions->operation_state != 49 &&
		conditions->operation_state != 50;
}

static void
print_conditions (const char *stage, const PentaxConditions *conditions)
{
	printf ("stage=%s state=%u activity=0x%08x exposure_mode=%u "
		"bulb_timer=%s bulb_seconds=%u/%u\n",
		stage, conditions->operation_state, conditions->activity_flags,
		conditions->exposure_mode,
		(conditions->capability_flags & PENTAX_CONDITION_BULB_TIMER) ? "yes" : "no",
		conditions->bulb_timer_seconds, conditions->bulb_timer_denominator);
}

static int
run_self_test (void)
{
	PentaxConditions conditions = {0};
	PentaxBulbProbeGuardResult result;
	int failures = 0;

	result = pentax_bulb_probe_validate_camera (0x25fb, 0x0189,
		PENTAX_MODEL_K3_MARK_III, 1, 1);
	if (result != PENTAX_BULB_PROBE_SAFE) failures++;
	if (pentax_bulb_probe_validate_camera (0x25fb, 0x017b,
		PENTAX_MODEL_K3_MARK_III, 1, 1) != PENTAX_BULB_PROBE_WRONG_CAMERA)
		failures++;
	if (pentax_bulb_probe_validate_camera (0x25fb, 0x0189,
		PENTAX_MODEL_K3_MARK_III, 1, 0) != PENTAX_BULB_PROBE_NOT_READY)
		failures++;
	if (pentax_bulb_probe_validate_camera (0x25fb, 0x0189,
		PENTAX_MODEL_K1_MARK_II, 1, 1) != PENTAX_BULB_PROBE_WRONG_CAMERA)
		failures++;

	conditions.exposure_mode = PENTAX_EXP_MODE_BULB;
	conditions.capability_flags = PENTAX_CONDITION_BULB_TIMER;
	conditions.bulb_timer_seconds = 1;
	conditions.bulb_timer_denominator = 1;
	if (pentax_bulb_probe_validate_conditions (&conditions) != PENTAX_BULB_PROBE_SAFE)
		failures++;
	conditions.bulb_timer_seconds = 2;
	if (pentax_bulb_probe_validate_conditions (&conditions) != PENTAX_BULB_PROBE_TIMER_TOO_LONG)
		failures++;
	conditions.bulb_timer_seconds = 1;
	conditions.exposure_mode = PENTAX_EXP_MODE_ASTROTRACER;
	if (pentax_bulb_probe_validate_conditions (&conditions) != PENTAX_BULB_PROBE_WRONG_MODE)
		failures++;
	conditions.exposure_mode = PENTAX_EXP_MODE_BULB;
	conditions.bulb_timer_seconds = 0;
	if (pentax_bulb_probe_validate_conditions (&conditions) != PENTAX_BULB_PROBE_TIMER_UNAVAILABLE)
		failures++;
	conditions.bulb_timer_seconds = 1;
	conditions.bulb_timer_denominator = 0;
	if (pentax_bulb_probe_validate_conditions (&conditions) != PENTAX_BULB_PROBE_TIMER_UNAVAILABLE)
		failures++;
	conditions.bulb_timer_denominator = 1;
	conditions.capability_flags = 0;
	if (pentax_bulb_probe_validate_conditions (&conditions) != PENTAX_BULB_PROBE_TIMER_UNAVAILABLE)
		failures++;
	conditions.capability_flags = PENTAX_CONDITION_BULB_TIMER;
	conditions.activity_flags = 1;
	if (pentax_bulb_probe_validate_conditions (&conditions) != PENTAX_BULB_PROBE_BUSY)
		failures++;
	conditions.activity_flags = 0;
	conditions.operation_state = 49;
	if (pentax_bulb_probe_validate_conditions (&conditions) != PENTAX_BULB_PROBE_BUSY)
		failures++;
	conditions.operation_state = 50;
	if (pentax_bulb_probe_validate_conditions (&conditions) != PENTAX_BULB_PROBE_BUSY)
		failures++;

	if (failures) {
		fprintf (stderr, "guard_self_test=fail failures=%d\n", failures);
		return 1;
	}
	puts ("guard_self_test=pass hardware_access=none");
	return 0;
}

int
main (int argc, char **argv)
{
	Camera *camera = NULL;
	GPContext *context = NULL;
	CameraAbilities abilities;
	PentaxConditions conditions;
	CameraFilePath path = {{0}, {0}};
	CameraFileInfo file_info;
	PentaxBulbProbeGuardResult guard_result;
	char confirmation[64];
	int result = GP_OK, initialized = 0, capture_attempted = 0;
	int execute_capture, exit_result = GP_OK;

	if ((argc == 2) && !strcmp (argv[1], "--self-test"))
		return run_self_test ();
	if ((argc != 4) ||
	    (strcmp (argv[1], "--preflight") && strcmp (argv[1], "--execute")) ||
	    strcmp (argv[2], EXPECTED_MODEL)) {
		fprintf (stderr, "usage: %s --self-test | --preflight \"%s\" usb:BUS,DEVICE | --execute \"%s\" usb:BUS,DEVICE\n",
			argv[0], EXPECTED_MODEL, EXPECTED_MODEL);
		return 2;
	}
	execute_capture = !strcmp (argv[1], "--execute");
	if (execute_capture && !isatty (STDIN_FILENO)) {
		fprintf (stderr, "refusing: hardware capture requires an interactive terminal\n");
		return 2;
	}

	context = gp_context_new ();
	if (!context)
		return 1;
	result = sample_open_camera (&camera, argv[2], argv[3], context);
	if (result < GP_OK)
		goto done;
	result = gp_camera_get_abilities (camera, &abilities);
	if (result < GP_OK)
		goto done;
	printf ("selected_model=%s usb=%04x:%04x port=%s\n", abilities.model,
		abilities.usb_vendor, abilities.usb_product, argv[3]);
	result = gp_camera_init (camera, context);
	if (result < GP_OK)
		goto done;
	initialized = 1;

	guard_result = pentax_bulb_probe_validate_camera (abilities.usb_vendor,
		abilities.usb_product, camera->pl->params.pentax.model_no,
		camera->pl->params.pentax.supported_model,
		camera->pl->params.pentax.vendor_mode_enabled);
	if (guard_result != PENTAX_BULB_PROBE_SAFE) {
		fprintf (stderr, "preflight=refused reason=%s\n",
			guard_result_name (guard_result));
		result = GP_ERROR_NOT_SUPPORTED;
		goto done;
	}
	result = read_conditions (camera, &conditions);
	if (result < GP_OK)
		goto done;
	print_conditions ("before", &conditions);
	guard_result = pentax_bulb_probe_validate_conditions (&conditions);
	if (guard_result != PENTAX_BULB_PROBE_SAFE) {
		fprintf (stderr, "preflight=refused reason=%s\n",
			guard_result_name (guard_result));
		result = GP_ERROR_CAMERA_BUSY;
		goto done;
	}
	if (!execute_capture) {
		puts ("preflight=pass capture=not-run hardware_access=read-only");
		goto done;
	}

	printf ("This will trigger exactly one K-3 III camera-timed Bulb capture "
		"with the camera-reported timer at or below one second.\n"
		"No settings will be changed, and no file will be downloaded or deleted.\n"
		"Type %s to continue: ", CONFIRMATION);
	fflush (stdout);
	if (!fgets (confirmation, sizeof (confirmation), stdin) ||
	    strcmp (confirmation, CONFIRMATION "\n")) {
		fprintf (stderr, "capture=cancelled\n");
		result = GP_ERROR_CANCEL;
		goto done;
	}

	capture_attempted = 1;
	result = gp_camera_capture (camera, GP_CAPTURE_IMAGE, &path, context);
	if (result < GP_OK) {
		fprintf (stderr, "capture=failed result=%d outcome=unknown; "
			"inspect the camera/card and do not retry\n", result);
		goto done;
	}
	printf ("capture=completed path=%s/%s\n", path.folder, path.name);
	if (!path.name[0]) {
		fprintf (stderr, "capture_path=missing outcome=unknown; do not retry\n");
		result = GP_ERROR_CORRUPTED_DATA;
		goto done;
	}
	memset (&file_info, 0, sizeof (file_info));
	result = gp_camera_file_get_info (camera, path.folder, path.name,
		&file_info, context);
	if ((result < GP_OK) || !(file_info.file.fields & GP_FILE_INFO_SIZE) ||
	    (file_info.file.size <= 0)) {
		fprintf (stderr, "captured_file=unverified outcome=unknown; "
			"preserve the card and do not retry\n");
		result = GP_ERROR_CORRUPTED_DATA;
		goto done;
	}
	printf ("captured_file_size=%ld\n", file_info.file.size);

	result = read_conditions (camera, &conditions);
	if (result < GP_OK || !conditions_idle (&conditions)) {
		fprintf (stderr, "post_capture=not_idle outcome=unknown; "
			"preserve the camera/card state and do not retry\n");
		result = GP_ERROR_CAMERA_BUSY;
		goto done;
	}
	print_conditions ("after", &conditions);
	puts ("bulb_timed_capture=pass captures=1 settings_changed=no files_deleted=no");

done:
	if (initialized) {
		exit_result = gp_camera_exit (camera, context);
		if ((result >= GP_OK) && (exit_result < GP_OK))
			result = exit_result;
	}
	if (camera)
		gp_camera_unref (camera);
	if (context)
		gp_context_unref (context);
	if (result < GP_OK) {
		fprintf (stderr, "probe_result=fail result=%d cleanup=%d",
			result, exit_result);
		if (capture_attempted)
			fprintf (stderr, " capture_outcome=unknown; do_not_retry");
		fputc ('\n', stderr);
		return 1;
	}
	return 0;
}
