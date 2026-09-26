#include "config.h"

#include <string.h>

#include <gphoto2/gphoto2-file.h>
#include <gphoto2/gphoto2-port-log.h>
#include <gphoto2/gphoto2-result.h>

#include "pentax-publication.h"

int
pentax_capture_publications_clear (PTPParams *params)
{
	int i;

	if (!params)
		return GP_ERROR_BAD_PARAMETERS;
	for (i = 0; i < params->pentax.capture_publication_count; i++) {
		if (params->pentax.capture_publications[i])
			gp_file_unref (params->pentax.capture_publications[i]);
		params->pentax.capture_publications[i] = NULL;
		memset (&params->pentax.capture_publication_paths[i], 0,
			sizeof (params->pentax.capture_publication_paths[i]));
	}
	params->pentax.capture_publication_count = 0;
	return GP_OK;
}

int
pentax_capture_publication_add (PTPParams *params, const CameraFilePath *path,
	CameraFile *file)
{
	int slot;

	if (!params || !path || !file)
		return GP_ERROR_BAD_PARAMETERS;
	slot = params->pentax.capture_publication_count;
	if (slot < 0 || slot >= (int)(sizeof (params->pentax.capture_publications) /
				       sizeof (params->pentax.capture_publications[0])))
		return GP_ERROR_FIXED_LIMIT_EXCEEDED;
	params->pentax.capture_publication_paths[slot] = *path;
	params->pentax.capture_publications[slot] = file;
	gp_file_ref (file);
	params->pentax.capture_publication_count = slot + 1;
	GP_LOG_D ("Pentax capture publication generation %llu retains %s/%s in slot %d",
		(unsigned long long)params->pentax.capture_publication_generation,
		path->folder, path->name, slot);
	return GP_OK;
}

CameraFile *
pentax_capture_publication_find (PTPParams *params, const char *folder,
	const char *filename)
{
	int i;

	if (!params || !params->pentax.vendor_mode_enabled || !folder || !filename)
		return NULL;
	for (i = 0; i < params->pentax.capture_publication_count; i++) {
		CameraFilePath *path = &params->pentax.capture_publication_paths[i];

		if (!strcmp (path->folder, folder) && !strcmp (path->name, filename))
			return params->pentax.capture_publications[i];
	}
	return NULL;
}
