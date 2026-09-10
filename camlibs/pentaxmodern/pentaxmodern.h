#ifndef PENTAXMODERN_H
#define PENTAXMODERN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gphoto2/gphoto2-camera.h>

/* Add PTP specific includes if necessary, by looking at camlibs/ptp2/ptp.h */
/* For now, assume gphoto2-camera.h and basic PTP ops are sufficient */

/* Forward declarations for Pentax-specific structures, if any. */
/* Prefer generic PTP structures. */

/* Function Prototypes */
int pentaxmodern_init(Camera *camera);
int pentaxmodern_exit(Camera *camera);
int pentaxmodern_capture(Camera *camera, CameraCaptureType type, CameraFilePath *path, GPContext *context);
int pentaxmodern_summary(Camera *camera, CameraText *summary, GPContext *context);
int pentaxmodern_abilities_list(CameraAbilitiesList *list, GPContext *context);

/* Prototypes for getting and setting PTP device properties if needed */
/* These might be wrapped or extended versions of generic PTP functions */

#endif /* PENTAXMODERN_H */
