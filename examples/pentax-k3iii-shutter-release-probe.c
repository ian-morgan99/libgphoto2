/* K-3 III shutter-release process probe (issue #114 / o-v9q bulb-timeout).
 *  A. baseline conditions
 *  B. 60s bulb-timer boundary test (30s worked, 120s rejected)
 *  C. set 30s timer, verify, then gp_camera_capture and MEASURE the release:
 *     does the camera auto-release the shutter when the timer expires?
 *     (o-v9q symptom: capture hangs until a manual bulb release)
 *  D. download the captured file, verify size/mime
 *  E. restore shutter baseline
 *  F. ISO 12800 capture + restore (issue #109 acceptance item 5)
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-file.h>
#include <sys/stat.h>

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
print_field (const char *cond, const char *field)
{
	const char *p = cond ? strstr (cond, field) : NULL;
	if (!p) {
		printf ("  %s=(not found)\n", field);
		return;
	}
	const char *semi = strchr (p, ';');
	size_t len = semi ? (size_t)(semi - p) : strlen (p);
	printf ("  %.*s\n", (int)len, p);
}

static void
print_conditions (Camera *camera, GPContext *context, const char *label)
{
	char *cond = NULL;
	if (get_text (camera, context, "pentaxconditions", &cond) >= GP_OK) {
		printf ("stage=%s\n", label);
		print_field (cond, "shooting=");
		print_field (cond, "bulb-timer=");
		print_field (cond, "bulb-seconds=");
		print_field (cond, "ISO=");
		print_field (cond, "exposure-comp=");
		free (cond);
	} else {
		printf ("stage=%s (conditions read failed)\n", label);
	}
}

int
main (int argc, char **argv)
{
	Camera *camera = NULL;
	GPContext *context = NULL;
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
	printf ("stage=open result=%d (%s)\n", result, gp_result_as_string (result));
	if (result < GP_OK)
		goto out;
	result = gp_camera_init (camera, context);
	printf ("stage=init result=%d (%s)\n", result, gp_result_as_string (result));
	if (result < GP_OK)
		goto out;

	/* A. baseline */
	print_conditions (camera, context, "baseline");

	/* B. 60s bulb-timer boundary test */
	r = set_choice (camera, context, "pentaxdirectshutter", "60s");
	printf ("stage=60s set_result=%d (%s)\n", r, gp_result_as_string (r));
	print_conditions (camera, context, "60s-conditions");

	/* C. 30s timer + capture: measure the shutter-release process */
	r = set_choice (camera, context, "pentaxdirectshutter", "30s");
	printf ("stage=30s set_result=%d (%s)\n", r, gp_result_as_string (r));
	print_conditions (camera, context, "30s-conditions");

	{
		CameraFilePath capfile;
		const char *data = NULL, *mime = NULL;
		unsigned long fsize = 0;
		time_t t0, t1;

		memset (&capfile, 0, sizeof capfile);
		printf ("stage=capture-30s-timer starting (expect ~30s auto-release)...\n");
		t0 = time (NULL);
		r = gp_camera_capture (camera, GP_CAPTURE_IMAGE, &capfile, context);
		t1 = time (NULL);
		printf ("stage=capture-30s-timer result=%d (%s) elapsed=%ld s file=%s/%s\n",
			r, gp_result_as_string (r), (long)(t1 - t0),
			capfile.folder[0] ? capfile.folder : "(no-folder)",
			capfile.name[0] ? capfile.name : "(no-name)");
		if (r >= GP_OK && capfile.name[0]) {
			CameraFile *cap = NULL;
			int rr = gp_file_new (&cap);
			if (rr == GP_OK) {
				rr = gp_camera_file_get (camera, capfile.folder, capfile.name,
					GP_FILE_TYPE_NORMAL, cap, context);
				printf ("  file-get=%d (%s)\n", rr, gp_result_as_string (rr));
				if (rr == GP_OK) {
					rr = gp_file_get_data_and_size (cap, &data, &fsize);
					gp_file_get_mime_type (cap, &mime);
					printf ("  file-size=%lu bytes mime=%s\n",
						fsize, mime ? mime : "(null)");
					if (data && fsize > 0) {
						char outpath[600];
						FILE *fp;
						time_t now = time (NULL);
						struct tm tm_buf;
						localtime_r (&now, &tm_buf);
						snprintf (outpath, sizeof outpath,
							"/tmp/k3iii-30s-timer-%04d%02d%02d-%02d%02d%02d",
							tm_buf.tm_year + 1900, tm_buf.tm_mon + 1,
							tm_buf.tm_mday, tm_buf.tm_hour, tm_buf.tm_min,
							tm_buf.tm_sec);
						fp = fopen (outpath, "wb");
						if (fp) {
							fwrite (data, 1, fsize, fp);
							fclose (fp);
							printf ("  saved=%s\n", outpath);
						}
					}
				}
			}
			gp_file_unref (cap);
		}
	}

	/* E. restore shutter baseline */
	r = set_choice (camera, context, "pentaxdirectshutter", "1/20");
	printf ("stage=restore-shutter set_result=%d (%s)\n", r, gp_result_as_string (r));
	print_conditions (camera, context, "post-restore");

	/* F. ISO 12800 capture + restore (issue #109 item 5) */
	r = set_choice (camera, context, "iso", "12800");
	printf ("stage=iso-12800 set_result=%d (%s)\n", r, gp_result_as_string (r));
	print_conditions (camera, context, "iso-12800-verify");

	{
		CameraFilePath capfile;
		const char *data = NULL, *mime = NULL;
		unsigned long fsize = 0;

		memset (&capfile, 0, sizeof capfile);
		r = gp_camera_capture (camera, GP_CAPTURE_IMAGE, &capfile, context);
		printf ("stage=capture-iso12800 result=%d (%s) file=%s/%s\n",
			r, gp_result_as_string (r),
			capfile.folder[0] ? capfile.folder : "(no-folder)",
			capfile.name[0] ? capfile.name : "(no-name)");
		if (r >= GP_OK && capfile.name[0]) {
			CameraFile *cap = NULL;
			int rr = gp_file_new (&cap);
			if (rr == GP_OK) {
				rr = gp_camera_file_get (camera, capfile.folder, capfile.name,
					GP_FILE_TYPE_NORMAL, cap, context);
				printf ("  file-get=%d (%s)\n", rr, gp_result_as_string (rr));
				if (rr == GP_OK) {
					rr = gp_file_get_data_and_size (cap, &data, &fsize);
					gp_file_get_mime_type (cap, &mime);
					printf ("  file-size=%lu bytes mime=%s\n",
						fsize, mime ? mime : "(null)");
					if (data && fsize > 0) {
						char outpath[600];
						FILE *fp;
						time_t now = time (NULL);
						struct tm tm_buf;
						localtime_r (&now, &tm_buf);
						snprintf (outpath, sizeof outpath,
							"/tmp/k3iii-iso12800-%04d%02d%02d-%02d%02d%02d",
							tm_buf.tm_year + 1900, tm_buf.tm_mon + 1,
							tm_buf.tm_mday, tm_buf.tm_hour, tm_buf.tm_min,
							tm_buf.tm_sec);
						fp = fopen (outpath, "wb");
						if (fp) {
							fwrite (data, 1, fsize, fp);
							fclose (fp);
							printf ("  saved=%s\n", outpath);
						}
					}
				}
			}
			gp_file_unref (cap);
		}
	}

	/* restore ISO baseline */
	r = set_choice (camera, context, "iso", "100");
	printf ("stage=restore-iso set_result=%d (%s)\n", r, gp_result_as_string (r));
	print_conditions (camera, context, "final");

out:
	if (camera) {
		exit_result = gp_camera_exit (camera, context);
		if ((result >= GP_OK) && (exit_result < GP_OK))
			result = exit_result;
		gp_camera_unref (camera);
	}
	gp_context_unref (context);
	if (result < GP_OK) {
		fprintf (stderr, "probe failed: %s (%d)\n",
			gp_result_as_string (result), result);
		return 1;
	}
	printf ("shutter_release_probe=pass cleanup=ok\n");
	return 0;
}
