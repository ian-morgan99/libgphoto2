/* Regression test for the Polaris gp_file_save() publication edge. */
#include "config.h"

#include <gphoto2/gphoto2-file.h>
#include <gphoto2/gphoto2-result.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int fail(const char *what) {
	fprintf(stderr, "capture-event test: %s\n", what);
	return 1;
}

int main(void) {
	char root[] = "/tmp/polaris-capture-events.XXXXXX";
	char media_root[512];
	char image[512];
	char non_jpeg[512];
	char ledger[512];
	char line[1024];
	char *data;
	CameraFile *file = NULL;
	FILE *fp;
	struct stat st;
	int ret;

	if (!mkdtemp(root)) return fail("mkdtemp");
	if (snprintf(image, sizeof(image), "%s/capture.jpg", root) >= (int)sizeof(image) ||
		snprintf(non_jpeg, sizeof(non_jpeg), "%s/capture.png", root) >= (int)sizeof(non_jpeg) ||
		snprintf(ledger, sizeof(ledger), "%s/events.tsv", root) >= (int)sizeof(ledger))
		return fail("path too long");
	if (snprintf(media_root, sizeof(media_root), "%s/", root) >= (int)sizeof(media_root))
		return fail("media root too long");
	if (setenv("POLARIS_CAPTURE_EVENT_MEDIA_ROOT", media_root, 1) != 0 ||
		setenv("POLARIS_CAPTURE_EVENT_LEDGER", ledger, 1) != 0)
		return fail("setenv");
	if (gp_file_new(&file) < 0) return fail("gp_file_new");
	data = malloc(6);
	if (!data) return fail("malloc");
	memcpy(data, "\xff\xd8ok\xff\xd9", 6);
	if (gp_file_set_data_and_size(file, data, 6) < 0)
		return fail("gp_file_set_data_and_size");
	ret = gp_file_save(file, image);
	if (ret < 0) return fail(gp_result_as_string(ret));
	if (stat(image, &st) != 0 || st.st_size != 6) return fail("saved JPEG");
	fp = fopen(ledger, "r");
	if (!fp || !fgets(line, sizeof(line), fp)) return fail("missing ledger row");
	fclose(fp);
	if (!strstr(line, image) || !strstr(line, "\t6\t")) return fail("bad ledger row");
	if (gp_file_save(file, non_jpeg) < 0) return fail("saved non-JPEG");
	fp = fopen(ledger, "r");
	if (!fp || !fgets(line, sizeof(line), fp) || fgets(line, sizeof(line), fp)) return fail("non-JPEG emitted an event");
	fclose(fp);

	gp_file_free(file);
	unlink(image);
	unlink(non_jpeg);
	unlink(ledger);
	rmdir(root);
	return 0;
}
