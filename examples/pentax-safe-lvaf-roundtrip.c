/* Exact-model, single-session K-1 II live-view AF-position (0xd036) write
 * qualification.  Issue #8: the K-1 II accepts d036 writes but reports the
 * geometry centre on read-back instead of echoing the requested spot, so a
 * structurally valid 8-byte response is sufficient proof the property is
 * honoured (the camlib encodes this in _put_Pentax_LiveViewAFPosition).
 *
 * Per the TA review (2026-09-10), a no-op baseline write only proves transport
 * safety.  This harness therefore performs:
 *   1. keep PC-LV running via pentaxpclvkeep + start live view with one frame,
 *   2. read the baseline AF position (GET must be structurally valid),
 *   3. a BOUNDED NON-BASELINE coordinate write (baseline + small offset, so the
 *      camera cannot simply echo its own state back as an ACK),
 *   4. a post-write GET that must remain structurally valid,
 *   5. a POST-WRITE preview frame -- proving PC-LV stayed alive ACROSS the d036
 *      call (the persistence question the no-op version could not answer),
 *   6. finally-style baseline restoration: once the non-baseline write is
 *      accepted, EVERY exit path (success, failed read-back, failed preview,
 *      any error) attempts exact baseline restoration before cleanup/reconnect;
 *      a restore failure makes an otherwise-passing run FAIL (a qualification
 *      harness must not print pass while admitting the AF spot was left moved),
 *      and is reported separately when the main test already failed.  A
 *      post-restore GET proves the property still honours structurally valid
 *      responses after restoration, so retained evidence shows the restore
 *      rather than only SET acceptance.
 * A `lvaf_roundtrip=pass` is transport + session-persistence qualification;
 * closing #8's HW-W gate additionally needs an independent observable AF-area
 * effect or a source-faithful trace (see issue comment).
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-file.h>
#include <gphoto2/gphoto2-widget.h>

#include "samples.h"

static int
read_lvaf (Camera *camera, GPContext *context, char **value)
{
	CameraWidget *widget = NULL;
	const char *v = NULL;
	int result;

	result = gp_camera_get_single_config (camera,
		"pentaxliveviewafposition", &widget, context);
	if (result < GP_OK)
		return result;
	result = gp_widget_get_value (widget, &v);
	if (result >= GP_OK && v)
		*value = strdup (v);
	gp_widget_free (widget);
	return result;
}

static int
write_lvaf (Camera *camera, GPContext *context, const char *value)
{
	CameraWidget *widget = NULL;
	int result;

	result = gp_camera_get_single_config (camera,
		"pentaxliveviewafposition", &widget, context);
	if (result < GP_OK)
		return result;
	result = gp_widget_set_value (widget, value);
	if (result >= GP_OK)
		result = gp_camera_set_single_config (camera,
			"pentaxliveviewafposition", widget, context);
	gp_widget_free (widget);
	return result;
}

static int
set_keep_live_view (Camera *camera, GPContext *context, const char *value)
{
	CameraWidget *widget = NULL;
	int result;

	result = gp_camera_get_single_config (camera, "pentaxpclvkeep", &widget,
		context);
	if (result < GP_OK)
		return result;
	result = gp_widget_set_value (widget, value);
	if (result >= GP_OK)
		result = gp_camera_set_single_config (camera, "pentaxpclvkeep",
			widget, context);
	gp_widget_free (widget);
	return result;
}

int
main (int argc, char **argv)
{
	Camera *camera = NULL;
	GPContext *context = NULL;
	char *baseline = NULL, *after = NULL;
	int initialized = 0;
	int result = GP_OK, exit_result;
	/* TA review: once the baseline has been changed by a non-baseline write,
	 * EVERY subsequent exit path must attempt exact restoration before
	 * cleanup/reconnect.  Set as soon as that write is accepted; consumed in
	 * the out: block so no failure path (read-back, preview, cancel) can leave
	 * the AF spot moved. */
	int baseline_changed = 0;

	if ((argc != 3) || strcmp (argv[1], "Pentax:K-1 Mark II (PTP mode)")) {
		fprintf (stderr,
			"usage: %s 'Pentax:K-1 Mark II (PTP mode)' usb:BUS,DEVICE\n",
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
	initialized = 1;

	/* Keep PC-LV running so LiveViewAreaInfo is valid for the d036 path. */
	result = set_keep_live_view (camera, context, "on");
	if (result < GP_OK) {
		fprintf (stderr, "stage=keep-live-view error=%s (%d)\n",
			gp_result_as_string (result), result);
		goto out;
	}
	printf ("stage=keep-live-view on\n");

	/* Start PC-LV with one preview frame. */
	{
		CameraFile *file = NULL;
		result = gp_file_new (&file);
		if (result < GP_OK)
			goto out;
		result = gp_camera_capture_preview (camera, file, context);
		gp_file_unref (file);
		if (result < GP_OK) {
			fprintf (stderr, "stage=start-lv error=%s (%d)\n",
				gp_result_as_string (result), result);
			goto out;
		}
		printf ("stage=start-lv ok\n");
	}

	result = read_lvaf (camera, context, &baseline);
	if (result < GP_OK) {
		fprintf (stderr, "stage=read-baseline error=%s (%d)\n",
			gp_result_as_string (result), result);
		goto out;
	}
	printf ("stage=baseline position=%s\n", baseline);

	/* Bounded non-baseline coordinate: offset the baseline by a small amount so
	 * the camera cannot satisfy the write by echoing its own state.  The camlib
	 * bounds the point against LiveViewAreaInfo, so keep the offset modest and
	 * clamp to sane live-view coordinates (the K-1 II LV area is well under
	 * 4096x4096). */
	{
		unsigned int bx = 0, by = 0;
		char target[32];

		if ((sscanf (baseline, "%u,%u", &bx, &by) != 2)) {
			fprintf (stderr, "stage=parse-baseline: unexpected format '%s'\n",
				baseline);
			result = GP_ERROR_CORRUPTED_DATA;
			goto out;
		}
		if (bx < 100U)
			bx += 50U;                       /* nudge off the centre */
		else
			bx -= 50U;
		if (by < 100U)
			by += 50U;
		else
			by -= 50U;
		snprintf (target, sizeof (target), "%u,%u", bx, by);

		result = write_lvaf (camera, context, target);
		if (result < GP_OK) {
			fprintf (stderr, "stage=write-nonbaseline error=%s (%d)\n",
				gp_result_as_string (result), result);
			goto out;
		}
		baseline_changed = 1;                 /* every later exit path restores */
		printf ("stage=write-nonbaseline %s accepted\n", target);

		result = read_lvaf (camera, context, &after);
		if (result < GP_OK) {
			fprintf (stderr, "stage=read-after error=%s (%d)\n",
				gp_result_as_string (result), result);
			goto out;
		}
		printf ("stage=read-after position=%s\n", after);

		/* Post-write preview: proves PC-LV survived the d036 call.  With
		 * pentaxpclvkeep ON, camera_capture_preview skips its per-frame
		 * teardown, so a valid frame here means the same live-view session
		 * is still usable (the persistence question from the TA review). */
		{
			CameraFile *file = NULL;
			const char *data = NULL, *mime = NULL;
			unsigned long fsize = 0;

			result = gp_file_new (&file);
			if (result < GP_OK)
				goto out;
			result = gp_camera_capture_preview (camera, file, context);
			if (result == GP_OK) {
				result = gp_file_get_data_and_size (file, &data, &fsize);
				if (result == GP_OK)
					result = gp_file_get_mime_type (file, &mime);
				if ((result == GP_OK) &&
				    (!data || (fsize < 4) || !mime ||
				     strcmp (mime, GP_MIME_JPEG) != 0))
					result = GP_ERROR_CORRUPTED_DATA;
			}
			gp_file_unref (file);
			if (result < GP_OK) {
				fprintf (stderr, "stage=post-write-preview error=%s (%d)\n",
					gp_result_as_string (result), result);
				goto out;
			}
			printf ("stage=post-write-preview ok bytes=%lu\n", fsize);
		}

	}

out:
	/* Finally-style restoration (TA review): if the non-baseline write was
	 * accepted, attempt exact baseline restoration on EVERY exit path --
	 * success, failed read-back, failed preview, or any other error.  A
	 * restoration failure is reported separately and does NOT hide the
	 * original test result; it only adds a distinct restore-failed marker so
	 * retained evidence can distinguish "test passed but AF spot left moved"
	 * from a clean pass. */
	if (baseline_changed && camera) {
		int rret = write_lvaf (camera, context, baseline);
		if (rret < GP_OK) {
			fprintf (stderr, "stage=restore-baseline FAILED: %s (%d) -- "
				"AF spot may be left at the non-baseline coordinate\n",
				gp_result_as_string (rret), rret);
			/* TA review: a qualification harness must not print pass / exit 0
			 * while admitting the AF spot was left moved.  Fold the restore
			 * failure into the final status ONLY when the main test path
			 * succeeded; an already-failed run keeps its original error so
			 * the restoration problem is reported separately, not as THE
			 * cause. */
			if (result >= GP_OK)
				result = rret;
		} else {
			printf ("stage=restore-baseline %s\n", baseline);
			/* Retained-evidence GET: prove the property still honours a
			 * structurally valid response AFTER restoration (the K-1 II
			 * reports the geometry centre rather than echoing coordinates,
			 * so structural validity is the model-correct criterion).  A
			 * failed post-restore GET means the session may be wedged even
			 * though the SET was accepted -- report it distinctly. */
			char *restored = NULL;
			int gret = read_lvaf (camera, context, &restored);
			if (gret < GP_OK) {
				fprintf (stderr, "stage=post-restore-read FAILED: %s (%d)\n",
					gp_result_as_string (gret), gret);
				if (result >= GP_OK)
					result = gret;
			} else {
				printf ("stage=post-restore-read position=%s\n", restored);
				free (restored);
			}
		}
	}

	if (initialized) {
		exit_result = gp_camera_exit (camera, context);
		if ((result >= GP_OK) && (exit_result < GP_OK))
			result = exit_result;
	}
	if (camera)
		gp_camera_unref (camera);
	gp_context_unref (context);
	free (baseline);
	free (after);
	if (result < GP_OK) {
		fprintf (stderr, "LV AF round trip failed: %s (%d)\n",
			gp_result_as_string (result), result);
		return 1;
	}
	printf ("lvaf_roundtrip=pass cleanup=ok\n");
	return 0;
}
