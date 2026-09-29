/* Per-thread operation context for the PTP camlib. */
#ifndef CAMLIBS_PTP2_PTP_CONTEXT_H
#define CAMLIBS_PTP2_PTP_CONTEXT_H

#include <gphoto2/gphoto2-library.h>

/* PTPParams->data is shared by all operations on a Camera. The operation
 * context is not: callers may invoke the same Camera from multiple threads.
 * Keep contexts thread-local so an overlapping operation cannot replace or
 * outlive the context used by an in-flight USB transfer. */
void ptp_context_set (void *owner, GPContext *context);
GPContext *ptp_context_get (void *owner);

#endif
