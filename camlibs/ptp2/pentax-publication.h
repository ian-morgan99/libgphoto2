#ifndef CAMLIBS_PTP2_PENTAX_PUBLICATION_H
#define CAMLIBS_PTP2_PENTAX_PUBLICATION_H

#include "ptp.h"

int pentax_capture_publications_clear (PTPParams *params);
int pentax_capture_publication_add (PTPParams *params,
	const CameraFilePath *path, CameraFile *file);
CameraFile *pentax_capture_publication_find (PTPParams *params,
	const char *folder, const char *filename);

#endif
