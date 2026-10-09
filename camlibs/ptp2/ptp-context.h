/* Per-thread operation context for the PTP camlib. */
#ifndef CAMLIBS_PTP2_PTP_CONTEXT_H
#define CAMLIBS_PTP2_PTP_CONTEXT_H

#include <gphoto2/gphoto2-library.h>

/* PTPParams->data is shared by all operations on a Camera. The operation
 * context is not: callers may invoke the same Camera from multiple threads.
 * Keep contexts thread-local so an overlapping operation cannot replace or
 * outlive the context used by an in-flight USB transfer. Each PTPData lifetime
 * receives a unique generation so a stale thread-local entry cannot match a
 * later session that reuses the same PTPData address. */
void ptp_context_owner_init (void *owner);
void ptp_context_set (void *owner, GPContext *context);
GPContext *ptp_context_get (void *owner);

/* Issue #190 opt-in diagnostics (GP_PTP_CONTEXT_PROBE=1). Read-only: they must
 * not take a reference or otherwise extend the lifetime of what they observe.
 * ptp_context_probe() reads the context fields by raw offset so it cannot fault
 * on a freed context -- only calling through the stored pointer can. */
int ptp_context_probe_enabled (void);
unsigned long ptp_context_thread_tag (void);
void ptp_context_probe (const char *where, void *owner, GPContext *context,
			unsigned long detail);

#endif
