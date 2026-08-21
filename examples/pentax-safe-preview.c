/* Read-only-in-memory Pentax preview smoke test. No capture or config setters. */
#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-file.h>

#include "samples.h"

int
main (int argc, char **argv)
{
	Camera *camera = NULL;
	GPContext *context = NULL;
	char *end = NULL;
	long requested;
	int initialized = 0, result = GP_OK, exit_result, i;

	if (argc != 4) {
		fprintf (stderr, "usage: %s MODEL usb:BUS,DEVICE FRAME_COUNT\n", argv[0]);
		return 2;
	}
	if (strcmp (argv[1], "Pentax:K-3 Mark III (MTP mode)") &&
	    strcmp (argv[1], "Pentax:K-1 Mark II (PTP mode)")) {
		fprintf (stderr, "MODEL is not an explicitly supported Pentax body\n");
		return 2;
	}
	errno = 0;
	requested = strtol (argv[3], &end, 10);
	if (errno || !end || *end || (requested < 1) || (requested > 500)) {
		fprintf (stderr, "FRAME_COUNT must be in the range 1..500\n");
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
	initialized = 1;

	for (i = 0; i < requested; i++) {
		CameraFile *file = NULL;
		const char *data = NULL, *mime = NULL;
		unsigned long size = 0;

		result = gp_file_new (&file);
		if (result < GP_OK) {
			fprintf (stderr, "frame=%d stage=file-new error=%s (%d)\n",
				i + 1, gp_result_as_string (result), result);
			goto out;
		}
		result = gp_camera_capture_preview (camera, file, context);
		if (result < GP_OK)
			fprintf (stderr, "frame=%d stage=capture-preview error=%s (%d)\n",
				i + 1, gp_result_as_string (result), result);
		if (result == GP_OK)
			result = gp_file_get_data_and_size (file, &data, &size);
		if (result == GP_OK)
			result = gp_file_get_mime_type (file, &mime);
		if ((result == GP_OK) &&
		    (!data || (size < 4) || !mime || strcmp (mime, GP_MIME_JPEG) ||
		     ((unsigned char)data[0] != 0xff) ||
		     ((unsigned char)data[1] != 0xd8) ||
		     ((unsigned char)data[size - 2] != 0xff) ||
		     ((unsigned char)data[size - 1] != 0xd9)))
			result = GP_ERROR_CORRUPTED_DATA;
		gp_file_unref (file);
		if (result < GP_OK)
			goto out;
		printf ("frame=%d bytes=%lu valid_jpeg=yes\n", i + 1, size);
		if (i + 1 < requested)
			usleep (250000);
	}

out:
	if (initialized) {
		exit_result = gp_camera_exit (camera, context);
		if ((result >= GP_OK) && (exit_result < GP_OK))
			result = exit_result;
	}
	if (camera)
		gp_camera_unref (camera);
	gp_context_unref (context);
	if (result < GP_OK) {
		fprintf (stderr, "safe preview failed: %s (%d)\n",
			gp_result_as_string (result), result);
		return 1;
	}
	printf ("completed_frames=%ld cleanup=ok\n", requested);
	return 0;
}
