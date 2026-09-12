/* test-pentax-aperture-alias.c
 *
 * Regression test for Pentax generic exposure compatibility aliases
 * (issue #53, umbrella #52). Drives the REAL public config-lookup path:
 * it dlopens the built ptp2 camlib and calls its exported
 * camera_get_single_config("aperture"), which routes through _get_config ->
 * have_prop/vendor matching -> widget construction -> GET. This proves the
 * alias is reachable through the public API with correct value semantics, not
 * merely that a menu entry points at the expected handlers.
 *
 * The test builds a Camera fixture (core gp_camera_new + ptp2's private-data
 * layout) whose PTP state is pre-seeded so no wire IO happens:
 *   - Pentax vendor id + pentax.vendor_mode_enabled  -> alias gate passes
 *   - FNumber descriptor pre-populated in the property cache -> GET resolves
 *     from the seeded value (no device needed)
 * and a second, non-Pentax fixture to confirm the alias stays fail-closed.
 */

#include "config.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <gphoto2/gphoto2-library.h>

/* ptp2 camlib private layout: PTPParams + the FNumber descriptor we seed. */
#include "ptp.h"
#include "ptp-private.h"

typedef int (*cam_get_single_config_fn)(Camera *camera, const char *confname,
                                       CameraWidget **widget, GPContext *context);

#define CHECK(expression) do { \
        if (!(expression)) { \
                fprintf (stderr, "FAIL line %d: %s\n", __LINE__, #expression); \
                return 1; \
        } \
} while (0)

/* Locate and dlopen the built ptp2 camlib via CAMLIBS (set by meson). */
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

/* Seed a PTPParams block for the fixture. */
static void
seed_pentax_params (PTPParams *params, int pentax, int advertise_ops)
{
	memset (params, 0, sizeof (*params));
	params->byteorder = PTP_DL_LE;
	params->cachetime = 2;

	if (pentax) {
		params->deviceinfo.VendorExtensionID = PTP_VENDOR_PENTAX;
		params->pentax.supported_model = 1;
		params->pentax.vendor_mode_enabled = 1;
	} else {
		params->deviceinfo.VendorExtensionID = PTP_VENDOR_CANON;
		params->pentax.supported_model = 0;
		params->pentax.vendor_mode_enabled = 0;
	}

	/* The generic property opcodes must be advertised so the opcode-based
	 * menu entries pass have_prop(). */
	if (advertise_ops) {
		uint16_t *ops = malloc (2 * sizeof (*ops));
		if (ops) {
			ops[0] = PTP_OC_GetDevicePropDesc;
			ops[1] = PTP_OC_GetDevicePropValue;
			params->deviceinfo.Operations = ops;
			params->deviceinfo.Operations_len = 2;
		}
	}

	/* Pre-seed the FNumber descriptor in the property cache so GET
	 * resolves from this value with no wire IO (cache-hit path). */
	PTPDevicePropDesc *dpd = calloc (3, sizeof (*dpd));
	if (dpd) {
		memset (dpd, 0, sizeof (*dpd));
		dpd->DevicePropCode = PTP_DPC_FNumber;
		dpd->DataType       = PTP_DTC_UINT16;
		dpd->GetSet         = PTP_DPGS_GetSet;
		dpd->FormFlag       = PTP_DPFF_Range;
		dpd->FORM.Range.MinValue.u16   = 400;   /* f/4.0 */
		dpd->FORM.Range.MaxValue.u16   = 2200;  /* f/22.0 */
		dpd->FORM.Range.StepSize.u16   = 100;
		dpd->CurrentValue.u16          = 800;   /* f/8.0 */
		dpd->timestamp                = time (NULL);
		dpd[1].DevicePropCode = PTP_DPC_PENTAX_ShutterSpeed;
		dpd[1].DataType       = PTP_DTC_UINT64;
		dpd[1].GetSet         = PTP_DPGS_GetSet;
		dpd[1].FormFlag       = PTP_DPFF_Enumeration;
		dpd[1].FORM.Enum.NumberOfValues = 2;
		dpd[1].FORM.Enum.SupportedValue = calloc (2, sizeof (PTPPropValue));
		if (dpd[1].FORM.Enum.SupportedValue) {
			dpd[1].FORM.Enum.SupportedValue[0].u64 = ((uint64_t)125 << 32) | 1;
			dpd[1].FORM.Enum.SupportedValue[1].u64 = ((uint64_t)500 << 32) | 1;
		}
		dpd[1].CurrentValue.u64 = ((uint64_t)125 << 32) | 1;
		dpd[1].timestamp = time (NULL);
		dpd[2].DevicePropCode = PTP_DPC_PENTAX_ExtendedISO;
		dpd[2].DataType       = PTP_DTC_UINT32;
		dpd[2].GetSet         = PTP_DPGS_GetSet;
		dpd[2].FormFlag       = PTP_DPFF_Enumeration;
		dpd[2].FORM.Enum.NumberOfValues = 2;
		dpd[2].FORM.Enum.SupportedValue = calloc (2, sizeof (PTPPropValue));
		if (dpd[2].FORM.Enum.SupportedValue) {
			dpd[2].FORM.Enum.SupportedValue[0].u32 = 200;
			dpd[2].FORM.Enum.SupportedValue[1].u32 = 400;
		}
		dpd[2].CurrentValue.u32 = 200;
		dpd[2].timestamp = time (NULL);
		params->dpd_cache.val = dpd;
		params->dpd_cache.len = 3;
	}
}

/* Build a Camera fixture with the given PTP state attached as private data. */
static int
build_fixture (Camera **out, GPContext *context, int pentax, int advertise_ops)
{
	Camera *camera;
	int ret = gp_camera_new (&camera);
	if (ret < GP_OK) {
		fprintf (stderr, "gp_camera_new failed: %s\n",
			 gp_result_as_string (ret));
		return 1;
	}

	CameraAbilities ab;
	memset (&ab, 0, sizeof (ab));
	ab.port = GP_PORT_USB;
	ab.usb_vendor = 0x25fb; /* Ricoh/Pentax */
	gp_camera_set_abilities (camera, ab);

	camera->pl = calloc (1, sizeof (CameraPrivateLibrary));
	if (!camera->pl) {
		fprintf (stderr, "out of memory for pl\n");
		return 1;
	}
	seed_pentax_params (&camera->pl->params, pentax, advertise_ops);

	/* Wire the context into the PTP data block as ptp2 expects. */
	camera->pl->params.data = calloc (1, sizeof (PTPData));
	if (!camera->pl->params.data) {
		fprintf (stderr, "out of memory for PTPData\n");
		return 1;
	}
	((PTPData *) camera->pl->params.data)->context = context;

	*out = camera;
	return 0;
}

int
main (void)
{
	void *handle = load_ptp2 ();
	if (!handle)
		return 77; /* skip */

	cam_get_single_config_fn get_single;
	{
		union { void *p; cam_get_single_config_fn f; } u;
		u.p = dlsym (handle, "camera_get_single_config");
		get_single = u.f;
	}
	if (!get_single) {
		fprintf (stderr, "FAIL: camera_get_single_config not exported (%s)\n",
			 dlerror());
		return 1;
	}

	GPContext *context = gp_context_new ();
	CHECK (context);

	/* --- Positive: Pentax fixture resolves "aperture" via public API. --- */
	Camera *cam = NULL;
	CHECK (!build_fixture (&cam, context, /*pentax=*/1, /*advertise_ops=*/1));
	CameraWidget *widget = NULL;
	int ret = get_single (cam, "aperture", &widget, context);
	CHECK (ret == GP_OK);
	CHECK (widget != NULL);

	const char *name = NULL;
	CHECK (gp_widget_get_name (widget, &name) == GP_OK);
	CHECK (name && !strcmp (name, "aperture"));

	/* Value semantics: the seeded FNumber range descriptor must flow into
	 * a RANGE widget whose value is CurrentValue/100.0 (f/8.0). */
	CameraWidgetType wtype;
	CHECK (gp_widget_get_type (widget, &wtype) == GP_OK);
	CHECK (wtype == GP_WIDGET_RANGE);
	float fval = 0.0f;
	CHECK (gp_widget_get_value (widget, &fval) == GP_OK);
	if (fval < 7.95f || fval > 8.05f) {
		fprintf (stderr, "FAIL: aperture value %f not ~8.0 (seeded f/8)\n", fval);
		return 1;
	}

	/* The generic shutter name must resolve through the Pentax direct
	 * descriptor path even though the property is absent from DeviceInfo. */
	CameraWidget *shutter = NULL;
	ret = get_single (cam, "shutterspeed", &shutter, context);
	CHECK (ret == GP_OK);
	CHECK (shutter != NULL);
	CHECK (gp_widget_get_name (shutter, &name) == GP_OK);
	CHECK (name && !strcmp (name, "shutterspeed"));
	CHECK (gp_widget_get_type (shutter, &wtype) == GP_OK);
	CHECK (wtype == GP_WIDGET_RADIO);
	char *shutter_value = NULL;
	CHECK (gp_widget_get_value (shutter, &shutter_value) == GP_OK);
	CHECK (shutter_value && !strcmp (shutter_value, "1/125"));

	/* ExtendedISO has the same discovery characteristic: the generic name
	 * must reach the direct descriptor handler without a DeviceInfo entry. */
	CameraWidget *iso = NULL;
	ret = get_single (cam, "iso", &iso, context);
	CHECK (ret == GP_OK);
	CHECK (iso != NULL);
	CHECK (gp_widget_get_name (iso, &name) == GP_OK);
	CHECK (name && !strcmp (name, "iso"));
	CHECK (gp_widget_get_type (iso, &wtype) == GP_OK);
	CHECK (wtype == GP_WIDGET_RADIO);
	char *iso_value = NULL;
	CHECK (gp_widget_get_value (iso, &iso_value) == GP_OK);
	CHECK (iso_value && !strcmp (iso_value, "200"));

	/* --- Negative: non-Pentax fixture (no generic property opcodes) stays
	 * fail-closed. Without the vendor match and without the advertised ops,
	 * have_prop() rejects every PENTAX-gated entry, so "aperture" is not even
	 * considered and the lookup misses. --- */
	Camera *cam2 = NULL;
	CHECK (!build_fixture (&cam2, context, /*pentax=*/0, /*advertise_ops=*/0));
	CameraWidget *widget2 = NULL;
	int ret2 = get_single (cam2, "aperture", &widget2, context);
	/* The direct-aperture handler refuses when vendor mode is off, so the
	 * single-config lookup must not yield a usable widget. */
	if (ret2 == GP_OK && widget2 != NULL) {
		fprintf (stderr, "FAIL: non-Pentax fixture unexpectedly exposed 'aperture'\n");
		return 1;
	}

	/* --- #69: menu-name uniqueness via the public list-config path. A
	 * duplicate registration would surface as two identical names in the
	 * camera_list_config output; assert exactly one occurrence on Pentax and
	 * none on a non-Pentax fixture (no new production export needed). */
	typedef int (*cam_list_config_fn)(Camera *camera, CameraList *list,
	                                  GPContext *context);
	{
		union { void *p; cam_list_config_fn f; } u;
		u.p = dlsym (handle, "camera_list_config");
		if (!u.f) {
			fprintf (stderr, "FAIL: camera_list_config not exported (%s)\n",
				 dlerror());
			return 1;
		}

		CameraList *list = NULL;
		CHECK (gp_list_new (&list) == GP_OK);
		CHECK (u.f (cam, list, context) == GP_OK);
		int count = gp_list_count (list);
		int occurrences = 0;
		for (int i = 0; i < count; i++) {
			const char *nm = NULL;
			if (gp_list_get_name (list, i, &nm) == GP_OK &&
			    nm && !strcmp (nm, "pentaxliveviewafposition"))
				occurrences++;
		}
		CHECK (occurrences == 1);
		gp_list_free (list);

		/* A non-Pentax fixture must not surface the entry at all: vendor
		 * matching fails, so have_prop() rejects the PENTAX-gated entry.
		 */
		CHECK (gp_list_new (&list) == GP_OK);
		CHECK (u.f (cam2, list, context) == GP_OK);
		count = gp_list_count (list);
		occurrences = 0;
		for (int i = 0; i < count; i++) {
			const char *nm = NULL;
			if (gp_list_get_name (list, i, &nm) == GP_OK &&
			    nm && !strcmp (nm, "pentaxliveviewafposition"))
				occurrences++;
		}
		CHECK (occurrences == 0);
		gp_list_free (list);
	}

	printf ("OK: public generic aperture, shutter and ISO aliases resolve seeded "
	       "Pentax descriptors, stay fail-closed for "
	       "non-Pentax, and pentaxliveviewafposition is registered exactly once (#69)\n");
	return 0;
}
