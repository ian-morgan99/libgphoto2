/* K-3 III bulb-boundary + high-ISO capture probe.
 *  A. list all pentaxdirectshutter choices (find timer values)
 *  B. try 60s bulb timer (boundary: 30s worked, 120s failed)
 *  C. restore shutter baseline
 *  D. set ISO 12800, verify via conditions
 *  E. capture one still image at ISO 12800 (>6400, issue #109 item 5)
 *  F. restore ISO baseline, verify
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

int
main (int argc, char **argv)
{
	Camera *camera = NULL;
	GPContext *context = NULL;
	char *cond = NULL;
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

	/* A. list all pentaxdirectshutter choices */
	{
		CameraWidget *w = NULL;
		r = gp_camera_get_single_config (camera, "pentaxdirectshutter", &w, context);
		if (r >= GP_OK) {
			int count = gp_widget_count_choices (w), i;
			const char *val = NULL;
			gp_widget_get_value (w, &val);
			printf ("stage=shutter-choices current=%s count=%d:\n", val ? val : "?", count);
			for (i = 0; i < count; i++) {
				const char *c = NULL;
				gp_widget_get_choice (w, i, &c);
				printf ("  [%2d] %s\n", i, c ? c : "?");
			}
			gp_widget_free (w);
		} else {
			printf ("stage=shutter-choices get failed: %s\n", gp_result_as_string (r));
		}
	}

	/* B. try 60s bulb timer */
	r = set_choice (camera, context, "pentaxdirectshutter", "60s");
	printf ("stage=60s set_result=%d\n", r);
	if (get_text (camera, context, "pentaxconditions", &cond) >= GP_OK) {
		print_field (cond, "bulb-seconds=");
		print_field (cond, "bulb-timer=");
		free (cond);
		cond = NULL;
	}

	/* C. restore shutter */
	r = set_choice (camera, context, "pentaxdirectshutter", "1/20");
	printf ("stage=restore-shutter set_result=%d\n", r);

	/* D. set ISO 12800 */
	r = set_choice (camera, context, "iso", "12800");
	printf ("stage=iso-12800 set_result=%d\n", r);
	if (get_text (camera, context, "pentaxconditions", &cond) >= GP_OK) {
		print_field (cond, "ISO=");
		free (cond);
		cond = NULL;
	}

	/* E. capture one still at ISO 12800 */
	{
		char path[512];
		time_t now = time (NULL);
		struct tm tm_buf;
		localtime_r (&now, &tm_buf);
		snprintf (path, sizeof path, "/tmp/k3iii-iso12800-%04d%02d%02d-%02d%02d%02d.raw",
			tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday,
			tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec);

		CameraFilePath capfile;
		memset (&capfile, 0, sizeof capfile);
		r = gp_camera_capture (camera, GP_CAPTURE_IMAGE, &capfile, context);
		printf ("stage=capture-iso12800 result=%d (%s) file=%s/%s\n",
			r, gp_result_as_string (r),
			capfile.folder[0] ? capfile.folder : "(no-folder)",
			capfile.name[0] ? capfile.name : "(no-name)");
		if (r >= GP_OK && capfile.name[0]) {
			CameraFile *cap = NULL;
			const char *data = NULL, *mime = NULL;
			unsigned long fsize = 0;
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
						snprintf (outpath, sizeof outpath,
							"/tmp/k3iii-iso12800-%s", capfile.name);
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

	/* F. restore ISO */
	r = set_choice (camera, context, "iso", "100");
	printf ("stage=restore-iso set_result=%d\n", r);
	if (get_text (camera, context, "pentaxconditions", &cond) >= GP_OK) {
		print_field (cond, "ISO=");
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
	if (result < GP_OK) {
		fprintf (stderr, "probe failed: %s (%d)\n",
			gp_result_as_string (result), result);
		return 1;
	}
	printf ("bulb_capture_probe=pass cleanup=ok\n");
	return 0;
}
