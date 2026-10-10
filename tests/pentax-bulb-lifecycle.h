#ifndef PENTAX_BULB_LIFECYCLE_H
#define PENTAX_BULB_LIFECYCLE_H

#include <gphoto2/gphoto2.h>

typedef struct {
	void *opaque;
	int (*set_edge) (void *opaque, int start);
	int (*check_cancel) (void *opaque);
	int (*wait) (void *opaque);
	int (*collect) (void *opaque);
} PentaxBulbLifecycleOps;

typedef struct {
	int start_attempted;
	int start_confirmed;
	int shutter_open;
	int explicit_stop_attempted;
	int explicit_stop_result;
	int cleanup_stop_attempted;
	int cleanup_stop_result;
	int collection_attempted;
	int collection_result;
	int cancelled_after_collection;
	int operator_intervention_required;
} PentaxBulbLifecycleState;

int pentax_bulb_run_lifecycle (const PentaxBulbLifecycleOps *ops,
	PentaxBulbLifecycleState *state);

#endif
