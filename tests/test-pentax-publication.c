/* Regression for the Polaris #145 capture-result handoff.
 *
 * Reproduce the shipped sequence without camera hardware:
 *   publish a memory-backed /IMGP*.JPG result -> refresh/reset the camera
 *   filesystem -> ordinary lookup returns -108 -> retrieve the same payload
 *   from the Pentax session/generation publication ledger.
 */
#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <gphoto2/gphoto2-file.h>
#include <gphoto2/gphoto2-filesys.h>
#include <gphoto2/gphoto2-result.h>

#include "pentax-publication.h"

static int
missing_file (CameraFilesystem *fs, const char *folder, const char *filename,
	CameraFileType type, CameraFile *file, void *data, GPContext *context)
{
	(void)fs; (void)folder; (void)filename; (void)type;
	(void)file; (void)data; (void)context;
	return GP_ERROR_FILE_NOT_FOUND;
}

int
main (void)
{
	static const char payload[] = "pentax-capture-payload";
	PTPParams params;
	CameraFilePath first, second;
	CameraFilesystem *fs = NULL;
	CameraFilesystemFuncs funcs;
	CameraFile *published = NULL, *received = NULL;
	CameraFile *retained;
	CameraList *list = NULL;
	char *owned;
	const char *data = NULL;
	unsigned long size = 0;
	int handled = 0, count = 0;

	memset (&params, 0, sizeof (params));
	memset (&first, 0, sizeof (first));
	memset (&second, 0, sizeof (second));
	memset (&funcs, 0, sizeof (funcs));
	params.pentax.vendor_mode_enabled = 1;
	params.pentax.capture_publication_generation = 1;
	strcpy (first.folder, "/");
	strcpy (first.name, "IMGP3634.JPG");
	strcpy (second.folder, "/");
	strcpy (second.name, "IMGP3635.JPG");

	assert (gp_file_new (&published) == GP_OK);
	owned = malloc (sizeof (payload));
	assert (owned != NULL);
	memcpy (owned, payload, sizeof (payload));
	assert (gp_file_set_data_and_size (published, owned, sizeof (payload)) == GP_OK);
	assert (pentax_capture_publication_add (&params, &first, published) == GP_OK);

	assert (gp_filesystem_new (&fs) == GP_OK);
	funcs.get_file_func = missing_file;
	assert (gp_filesystem_set_funcs (fs, &funcs, NULL) == GP_OK);
	assert (gp_filesystem_append (fs, first.folder, first.name, NULL) == GP_OK);
	assert (gp_filesystem_set_file_noop (fs, first.folder, first.name,
		GP_FILE_TYPE_NORMAL, published, NULL) == GP_OK);
	gp_file_unref (published);
	published = NULL;

	/* This is the pre-fix failure: Benro refreshes the returned folder and the
	 * virtual filesystem no longer owns the only payload. */
	assert (gp_filesystem_reset (fs) == GP_OK);
	assert (gp_file_new (&received) == GP_OK);
	assert (gp_filesystem_get_file (fs, first.folder, first.name,
		GP_FILE_TYPE_NORMAL, received, NULL) == GP_ERROR_FILE_NOT_FOUND);

	/* The production Pentax callback now serves the generation-owned copy. */
	retained = pentax_capture_publication_find_for_type (&params, first.folder,
		first.name, GP_FILE_TYPE_NORMAL);
	assert (retained != NULL);
	/* Drive the same selector used by get_file_func: derivative requests must
	 * not receive the retained normal payload. */
	assert (pentax_capture_publication_find_for_type (&params, first.folder,
		first.name, GP_FILE_TYPE_PREVIEW) == NULL);
	assert (pentax_capture_publication_find_for_type (&params, first.folder,
		first.name, GP_FILE_TYPE_EXIF) == NULL);
	assert (pentax_capture_publication_find_for_type (&params, first.folder,
		first.name, GP_FILE_TYPE_RAW) == NULL);
	assert (gp_file_copy (received, retained) == GP_OK);
	assert (gp_file_get_data_and_size (received, &data, &size) == GP_OK);
	assert (size == sizeof (payload));
	assert (!memcmp (data, payload, sizeof (payload)));

	/* Starting generation N+1 releases N and cannot return a same-session stale
	 * result under either its old or a coincidentally reused filename. */
	assert (pentax_capture_publications_clear (&params) == GP_OK);
	params.pentax.capture_publication_generation++;
	assert (pentax_capture_publication_find (&params, first.folder, first.name) == NULL);
	assert (params.pentax.capture_publication_count == 0);

	assert (gp_file_new (&published) == GP_OK);
	owned = malloc (sizeof (payload));
	assert (owned != NULL);
	memcpy (owned, payload, sizeof (payload));
	assert (gp_file_set_data_and_size (published, owned, sizeof (payload)) == GP_OK);
	assert (pentax_capture_publication_add (&params, &second, published) == GP_OK);
	assert (pentax_capture_publication_find (&params, first.folder, first.name) == NULL);
	assert (pentax_capture_publication_find (&params, second.folder, second.name) != NULL);

	/* A deleted virtual publication must be dropped from the ledger so a later
	 * filesystem refresh cannot resurrect it (review fix #2).  Removing an entry
	 * that is not retained is a harmless no-op. */
	assert (gp_list_new (&list) == GP_OK);
	assert (pentax_capture_publications_list (&params, list) == GP_OK);
	assert (gp_list_count (list) == 1);
	assert (pentax_capture_publication_delete_virtual (&params, second.folder,
		second.name, &handled) == GP_OK);
	assert (handled == 1);
	assert (pentax_capture_publication_find (&params, second.folder, second.name) == NULL);
	assert (params.pentax.capture_publication_count == 0);
	/* The production root-list helper is also what refresh calls. Deleted
	 * publications must not reappear after resetting/relisting the filesystem. */
	assert (gp_filesystem_reset (fs) == GP_OK);
	assert (gp_list_reset (list) == GP_OK);
	assert (pentax_capture_publications_list (&params, list) == GP_OK);
	assert (gp_list_count (list) == 0);
	handled = 99;
	assert (pentax_capture_publication_delete_virtual (&params, "/store_00000001",
		"physical.JPG", &handled) == GP_OK);
	assert (handled == 0);

	gp_file_unref (published);
	gp_file_unref (received);
	gp_list_free (list);
	pentax_capture_publications_clear (&params);
	gp_filesystem_free (fs);
	return 0;
}
