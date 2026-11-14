/* test-pentax-keep-live-view-toggle.c
 *
 * Regression test for the pentaxpclvkeep toggle contract (issue #141 review
 * feedback on 43dccb81b).
 *
 * _get_Pentax_KeepLiveView() builds a GP_WIDGET_TOGGLE and
 * _put_Pentax_KeepLiveView() reads it with gp_widget_get_value(widget, &int).
 * gp_widget_set_value() dereferences its argument as int* for a toggle, so a
 * caller that passes a string literal ("on"/"off") has the pointer's bytes read
 * as an integer: "off" is nonzero and silently sets keep_live_view=1 while
 * returning GP_OK.  A cleanup path that logs "live-view-keep=off result=0" then
 * lies about the state it left the camera in.
 *
 * This drives the REAL public config path -- it dlopens the built ptp2 camlib
 * and calls camera_get_single_config / camera_set_single_config for
 * "pentaxpclvkeep", so it proves the handler's value semantics rather than
 * merely that a menu entry exists.  The toggle is pure session state, so no
 * wire IO and no device is needed.
 */

#include "config.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gphoto2/gphoto2-library.h>

/* ptp2 camlib private layout, for the seeded PTPParams fixture. */
#include "ptp.h"
#include "ptp-private.h"
#include "pentax-utils.h"

typedef int (*cam_get_single_config_fn)(Camera *camera, const char *confname,
                                       CameraWidget **widget, GPContext *context);
typedef int (*cam_set_single_config_fn)(Camera *camera, const char *confname,
                                       CameraWidget *widget, GPContext *context);

#define CHECK(expression) do { \
	if (!(expression)) { \
		fprintf (stderr, "FAIL line %d: %s\n", __LINE__, #expression); \
		return 1; \
	} \
} while (0)

static void *
load_ptp2 (void)
{
	const char *camlibs = getenv ("CAMLIBS");
	char *candidate;
	void *handle = NULL;

	if (!camlibs || !*camlibs) {
		fprintf (stderr, "SKIP: CAMLIBS not set\n");
		return NULL;
	}
	for (char *entry = strtok (strdup (camlibs), ":"); entry;
	     entry = strtok (NULL, ":")) {
		candidate = malloc (strlen (entry) + strlen ("/ptp2.so") + 1);
		if (!candidate)
			return NULL;
		strcpy (candidate, entry);
		strcat (candidate, "/ptp2.so");
		handle = dlopen (candidate, RTLD_NOW | RTLD_LOCAL);
		free (candidate);
		if (handle)
			break;
	}
	if (!handle) {
		fprintf (stderr, "SKIP: no ptp2.so under CAMLIBS=%s (%s)\n",
			 camlibs, dlerror());
		return NULL;
	}
	return handle;
}

/* Pentax fixture: the toggle is gated on supported_model + vendor_mode_enabled,
 * and its menu entry carries propid 0 with type PTP_OC_GetDevicePropValue, so
 * have_prop() only matches when that opcode is advertised in DeviceInfo. */
static int
build_fixture (Camera **out, GPContext *context)
{
	Camera *camera;
	CameraAbilities ab;
	uint16_t *ops;

	if (gp_camera_new (&camera) < GP_OK) {
		fprintf (stderr, "gp_camera_new failed\n");
		return 1;
	}
	memset (&ab, 0, sizeof (ab));
	ab.port = GP_PORT_USB;
	ab.usb_vendor = 0x25fb; /* Ricoh/Pentax */
	gp_camera_set_abilities (camera, ab);

	camera->pl = calloc (1, sizeof (CameraPrivateLibrary));
	if (!camera->pl) {
		fprintf (stderr, "out of memory for pl\n");
		return 1;
	}
	camera->pl->params.byteorder = PTP_DL_LE;
	camera->pl->params.deviceinfo.VendorExtensionID = PTP_VENDOR_PENTAX;
	camera->pl->params.pentax.supported_model = 1;
	camera->pl->params.pentax.vendor_mode_enabled = 1;
	camera->pl->params.pentax.model_no = PENTAX_MODEL_K1_MARK_II;

	ops = calloc (2, sizeof (*ops));
	if (!ops) {
		fprintf (stderr, "out of memory for ops\n");
		return 1;
	}
	ops[0] = PTP_OC_GetDevicePropDesc;
	ops[1] = PTP_OC_GetDevicePropValue;
	camera->pl->params.deviceinfo.Operations = ops;
	camera->pl->params.deviceinfo.Operations_len = 2;

	/* The camlib operations install their context through params.data. */
	camera->pl->params.data = calloc (1, sizeof (PTPData));
	if (!camera->pl->params.data) {
		fprintf (stderr, "out of memory for PTPData\n");
		return 1;
	}
	(void)context;
	*out = camera;
	return 0;
}

/* The exact pattern the benchmark helper must use: an int, not a string. */
static int
set_toggle (cam_get_single_config_fn get_single, cam_set_single_config_fn set_single,
            Camera *camera, GPContext *context, int enabled)
{
	CameraWidget *widget = NULL;
	int result = get_single (camera, "pentaxpclvkeep", &widget, context);

	if (result >= GP_OK)
		result = gp_widget_set_value (widget, &enabled);
	if (result >= GP_OK)
		result = set_single (camera, "pentaxpclvkeep", widget, context);
	if (widget)
		gp_widget_free (widget);
	return result;
}

static int
get_toggle (cam_get_single_config_fn get_single, Camera *camera,
            GPContext *context, int *enabled)
{
	CameraWidget *widget = NULL;
	int result = get_single (camera, "pentaxpclvkeep", &widget, context);

	if (result >= GP_OK)
		result = gp_widget_get_value (widget, enabled);
	if (widget)
		gp_widget_free (widget);
	return result;
}

int
main (void)
{
	void *handle = load_ptp2 ();
	cam_get_single_config_fn get_single;
	cam_set_single_config_fn set_single;
	union { void *p; cam_get_single_config_fn g; } ug;
	union { void *p; cam_set_single_config_fn s; } us;
	GPContext *context;
	Camera *cam = NULL;
	CameraWidget *widget = NULL;
	CameraWidgetType wtype;
	int value = -1;
	int ret;

	if (!handle)
		return 77; /* skip */

	ug.p = dlsym (handle, "camera_get_single_config");
	get_single = ug.g;
	us.p = dlsym (handle, "camera_set_single_config");
	set_single = us.s;
	if (!get_single || !set_single) {
		fprintf (stderr, "FAIL: single-config entry points not exported (%s)\n",
			 dlerror());
		return 1;
	}

	context = gp_context_new ();
	CHECK (context);
	CHECK (!build_fixture (&cam, context));

	/* The widget must be a TOGGLE: that is what makes the int-vs-string
	 * distinction a correctness issue rather than a cosmetic one. */
	ret = get_single (cam, "pentaxpclvkeep", &widget, context);
	CHECK (ret == GP_OK);
	CHECK (widget != NULL);
	CHECK (gp_widget_get_type (widget, &wtype) == GP_OK);
	CHECK (wtype == GP_WIDGET_TOGGLE);
	CHECK (gp_widget_get_value (widget, &value) == GP_OK);
	CHECK (value == 0); /* fresh session: not keeping live view */
	gp_widget_free (widget);
	widget = NULL;

	/* 0 -> 1 -> 0 through the public set path, each verified by a fresh GET. */
	CHECK (set_toggle (get_single, set_single, cam, context, 1) == GP_OK);
	CHECK (get_toggle (get_single, cam, context, &value) == GP_OK);
	CHECK (value == 1);

	CHECK (set_toggle (get_single, set_single, cam, context, 0) == GP_OK);
	CHECK (get_toggle (get_single, cam, context, &value) == GP_OK);
	CHECK (value == 0);

	/* The hazard the review reported: gp_widget_set_value dereferences int* for
	 * a toggle, so a string literal is read as its bytes.  "off" therefore
	 * reads back as enabled -- GP_OK plus a logged "off" is not evidence the
	 * control was cleared.  This asserts the core-library semantics that make
	 * the int argument mandatory, so a future caller cannot regress silently. */
	ret = get_single (cam, "pentaxpclvkeep", &widget, context);
	CHECK (ret == GP_OK);
	CHECK (gp_widget_set_value (widget, "off") == GP_OK);
	CHECK (gp_widget_get_value (widget, &value) == GP_OK);
	CHECK (value != 0);
	gp_widget_free (widget);
	widget = NULL;

	/* And the handler really does consume that nonzero value as "keep live
	 * view on", which is why the string form is unsafe rather than merely
	 * untidy. */
	CHECK (set_toggle (get_single, set_single, cam, context, 1) == GP_OK);
	CHECK (get_toggle (get_single, cam, context, &value) == GP_OK);
	CHECK (value == 1);

	/* A function-backed control that is unavailable must propagate the
	 * handler's error.  Returning GP_OK with a NULL widget makes callers
	 * report a misleading GP_ERROR_BAD_PARAMETERS instead.  The K-1 II
	 * focus-peaking getter is deliberately unavailable until this session
	 * owns PC live view, so this is also the exact guard exercised by the
	 * recent remote read-only qualification. */
	ret = get_single (cam, "pentaxfocuspeaking", &widget, context);
	CHECK (ret == GP_ERROR_NOT_SUPPORTED);
	CHECK (widget == NULL);

	/* Non-Pentax sessions must remain fail-closed on the existing toggle. */
	{
		Camera *other = NULL;
		CameraWidget *other_widget = NULL;

		CHECK (!build_fixture (&other, context));
		other->pl->params.deviceinfo.VendorExtensionID = PTP_VENDOR_CANON;
		other->pl->params.pentax.supported_model = 0;
		other->pl->params.pentax.vendor_mode_enabled = 0;
		ret = get_single (other, "pentaxpclvkeep", &other_widget, context);
		CHECK (ret == GP_ERROR_NOT_SUPPORTED);
		CHECK (other_widget == NULL);
	}

	printf ("test-pentax-keep-live-view-toggle: all checks passed\n");
	return 0;
}
