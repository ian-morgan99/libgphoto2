#ifndef CAMLIBS_PTP2_PENTAX_PUBLICATION_H
#define CAMLIBS_PTP2_PENTAX_PUBLICATION_H

#include "ptp.h"

int pentax_capture_publications_clear (PTPParams *params);
int pentax_capture_publication_add (PTPParams *params,
	const CameraFilePath *path, CameraFile *file);
CameraFile *pentax_capture_publication_find (PTPParams *params,
	const char *folder, const char *filename);
CameraFile *pentax_capture_publication_find_for_type (PTPParams *params,
	const char *folder, const char *filename, CameraFileType type);
int pentax_capture_publication_remove (PTPParams *params,
	const char *folder, const char *filename);
int pentax_capture_publication_delete_virtual (PTPParams *params,
	const char *folder, const char *filename, int *handled);
int pentax_capture_publications_list (PTPParams *params, CameraList *list);

#endif
