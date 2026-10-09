/* Per-thread operation context for the PTP camlib. */
#ifndef CAMLIBS_PTP2_PTP_CONTEXT_H
#define CAMLIBS_PTP2_PTP_CONTEXT_H

#include <gphoto2/gphoto2-library.h>

/* PTPParams->data is shared by all operations on a Camera. The operation
 * context is not: callers may invoke the same Camera from multiple threads.
 * Keep contexts thread-local so an overlapping operation cannot replace or
 * outlive the context used by an in-flight USB transfer. Each PTPData lifetime
 * receives a unique generation so a stale thread-local entry cannot match a
 * later session that reuses the same PTPData address.
 *
 * Lifetime (issue #190): ptp_context_set() takes a reference on the context it
 * stores and releases it when the slot is cleared or reclaimed, so a stored
 * context cannot be freed while a transfer still holds the bound pointer.
 *
 * Ownership boundary, stated honestly: the caller (an operation entry point in
 * library.c or config.c) passes a context that is live at that instant, because
 * the frontend is inside a gp_camera_* call and has not reached its teardown.
 * That is the last point at which liveness is guaranteed, and it is where the
 * reference is taken.
 *
 * What the reference buys, and what it does not. Once stored here it prevents
 * the app's teardown from freeing the object while a binding refers to it,
 * provided the app's decrement observes our increment. It cannot order the
 * app's release: the closed Polaris app statically links its own copy of
 * gp_context_unref, which does a plain (non-atomic) decrement of the same
 * field, so neither a camlib lock nor our own atomic increment can serialise
 * it. The residual race is therefore an app read-modify-write that loaded the
 * count before our increment and stores back after it -- a lost update that
 * could still free the object under us.
 *
 * Closing that window completely is an integration-side change: the app must
 * not release its context while an operation it started is still in flight.
 * This module makes the lifetime correct on its own side and documents the
 * remaining dependency rather than pretending the lock covers the app. */
void ptp_context_owner_init (void *owner);
void ptp_context_set (void *owner, GPContext *context);
GPContext *ptp_context_get (void *owner);

/* Issue #190 opt-in diagnostics (GP_PTP_CONTEXT_PROBE=1).
 *
 * ptp_context_probe() logs identity only -- pointers, thread tag, owner
 * generation, transition name. It never dereferences the context, so it is safe
 * at a binding transition where ownership is uncertain.
 *
 * ptp_context_probe_owned() additionally reports the reference count, and may
 * only be called where this module demonstrably holds a reference. It reads the
 * count through the core accessor rather than a copied field layout. */
int ptp_context_probe_enabled (void);
unsigned long ptp_context_thread_tag (void);
void ptp_context_probe (const char *where, void *owner, GPContext *context,
			unsigned long detail);
void ptp_context_probe_owned (const char *where, void *owner,
			      GPContext *context, unsigned long detail);

/* How many references this module currently holds across all thread-local
 * tables. Used to prove the ref/unref pairing stays balanced across error,
 * cancel, generation-reclaim and thread-exit paths. Returns 0 once every
 * binding has been cleared. */
unsigned int ptp_context_held_refs (void);

/* Release every binding held by the calling thread. The table is thread-local,
 * so this reaches only the caller's own entries; call it on the way out of a
 * thread whose operations bound a context. */
void ptp_context_release_thread (void);

#endif
