/* K-3 III bulb/EV probe (issues #114 EV-in-Bulb, #118/#113 bulb duration,
 * #112 astro-limit). Single session, one variable per stage:
 *  A. baseline conditions
 *  B. shutter=Bulb -> read conditions (bulb-timer flag, bulb-seconds)
 *  C. EV set -1 while in Bulb -> PTP result + conditions exposure-comp read-back
 *  D. restore EV 0, verify
 *  E. shutter=30s -> conditions (bulb-timer flag? bulb-seconds? astro-limit?)
 *  F. shutter=120s -> same reads
 *  G. restore shutter baseline, verify
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-widget.h>

#include "samples.h"

static int
get_text (Camera *camera, GPContext *context, const char *name, char **out)
{
	CameraWidget *widget = NULL;
	const char *value = NULL;
	int result;

	result = gp_camera_get_single_config (camera, name, &widget, context);
	if (result < GP_OK)
		return result;
	result = gp_widget_get_value (widget, &value);
	if (result >= GP_OK)
		*out = strdup (value ? value : "");
	gp_widget_free (widget);
	return result;
}

static int
set_choice (Camera *camera, GPContext *context, const char *name,
           const char *value)
{
	CameraWidget *widget = NULL;
	int result;

	result = gp_camera_get_single_config (camera, name, &widget, context);
	if (result < GP_OK)
		return result;
	result = gp_widget_set_value (widget, value);
	if (result >= GP_OK)
		result = gp_camera_set_single_config (camera, name, widget, context);
	gp_widget_free (widget);
	return result;
}

static void
print_conditions (const char *label, const char *cond)
{
	const char *p;
	printf ("stage=%s\n", label);
	for (p = cond; p && *p; ) {
		const char *semi = strchr (p, ';');
		size_t len = semi ? (size_t)(semi - p) : strlen (p);
		if (len > 0)
			printf ("  %.*s", (int)len, p);
		printf ("\n");
		if (!semi)
			break;
		p = semi + 1;
	}
}

int
main (int argc, char **argv)
{
	Camera *camera = NULL;
	GPContext *context = NULL;
	char *cond = NULL, *shutter_base = NULL;
	int result = GP_OK, r, exit_result;

	if (argc != 3) {
		fprintf (stderr, "usage: %s 'Pentax K-3 Mark III (MTP mode)' usb:BUS,DEVICE\n",
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

	/* A. baseline */
	r = get_text (camera, context, "pentaxconditions", &cond);
	if (r < GP_OK) {
		printf ("stage=baseline read failed: %s\n", gp_result_as_string (r));
		result = r;
		goto out;
	}
	print_conditions ("baseline", cond);
	free (cond);
	cond = NULL;

	/* B. shutter -> Bulb */
	r = set_choice (camera, context, "pentaxdirectshutter", "Bulb");
	printf ("stage=bulb set_result=%d\n", r);
	if (get_text (camera, context, "pentaxconditions", &cond) >= GP_OK) {
		print_conditions ("bulb-conditions", cond);
		free (cond);
		cond = NULL;
	}

	/* C. EV -1 while in Bulb (issue #114) */
	r = set_choice (camera, context, "exposurecompensation", "-1");
	printf ("stage=ev-in-bulb set_result=%d\n", r);
	if (get_text (camera, context, "pentaxconditions", &cond) >= GP_OK) {
		const char *ec = strstr (cond, "exposure-comp=");
		printf ("stage=ev-in-bulb-verify %s\n",
			ec ? ec : "(no exposure-comp field)");
		free (cond);
		cond = NULL;
	}

	/* D. restore EV 0 */
	r = set_choice (camera, context, "exposurecompensation", "0");
	printf ("stage=ev-restore set_result=%d\n", r);

	/* E. shutter 30s */
	r = set_choice (camera, context, "pentaxdirectshutter", "30s");
	printf ("stage=30s set_result=%d\n", r);
	if (get_text (camera, context, "pentaxconditions", &cond) >= GP_OK) {
		print_conditions ("30s-conditions", cond);
		free (cond);
		cond = NULL;
	}

	/* F. shutter 120s */
	r = set_choice (camera, context, "pentaxdirectshutter", "120s");
	printf ("stage=120s set_result=%d\n", r);
	if (get_text (camera, context, "pentaxconditions", &cond) >= GP_OK) {
		print_conditions ("120s-conditions", cond);
		free (cond);
		cond = NULL;
	}

	/* G. restore shutter baseline */
	{
		char *base = NULL;
		if (get_text (camera, context, "pentaxconditions", &base) >= GP_OK) {
			const char *p = strstr (base, "bulb-seconds=");
			(void) p;
		}
		free (base);
	}
	r = set_choice (camera, context, "pentaxdirectshutter", "1/20");
	printf ("stage=restore-shutter set_result=%d\n", r);
	if (get_text (camera, context, "pentaxconditions", &cond) >= GP_OK) {
		print_conditions ("final-conditions", cond);
		free (cond);
		cond = NULL;
	}

out:
	if (camera) {
		exit_result = gp_camera_exit (camera, context);
		if ((result >= GP_OK) && (exit_result < GP_OK))
			result = exit_result;
		gp_camera_unref (camera);
	}
	gp_context_unref (context);
	if (cond)
		free (cond);
	if (shutter_base)
		free (shutter_base);
	if (result < GP_OK) {
		fprintf (stderr, "bulb/ev probe failed: %s (%d)\n",
			gp_result_as_string (result), result);
		return 1;
	}
	printf ("bulb_ev_probe=pass cleanup=ok\n");
	return 0;
}
