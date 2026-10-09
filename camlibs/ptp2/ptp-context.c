#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <gphoto2/gphoto2-library.h>

#include "ptp.h"
#include "ptp-context.h"
#include "ptp-private.h"

#define PTP_CONTEXT_BINDINGS 64

typedef struct {
	void *owner;
	uint32_t generation;
	GPContext *context;
} PTPContextBinding;

/* This module is part of ptp2.so, so the same TLS table is shared by the
 * operation entry points and the USB transport callbacks in that module. */
static __thread PTPContextBinding bindings[PTP_CONTEXT_BINDINGS];
static volatile uint32_t next_context_generation;

/* --- issue #190 opt-in context-lifetime probe -------------------------------
 *
 * The observed fault is an indirect call through a non-NULL but invalid
 * context->progress_start_func while a multi-megabyte object is downloading.
 * The static evidence says the closed app frees its only GPContext from
 * gp_params_exit (reached from cameraRest and from many cameraInit error
 * paths) while an operation started by an earlier attempt is still in flight,
 * and that nothing anywhere holds a reference: gp_context_ref is never called.
 *
 * This probe exists to confirm or refute that on hardware. It is deliberately
 * read-only: it must not take a reference, allocate, or otherwise extend the
 * lifetime of anything it observes, or it would hide the very defect it is
 * looking for.
 *
 * It is also deliberately limited to *identity*: pointers, thread tag, owner
 * generation and the transition name. It does not read any field of the
 * context. At the places it is called the whole question is whether the object
 * is still alive, and reading a field of a freed object is undefined behaviour
 * even when the page happens to still be mapped -- it can fault, be raced by
 * reuse, or report a value that was never real. Anything that wants field
 * values must use ptp_context_probe_owned(), which is only called where this
 * module demonstrably holds a reference.
 *
 * Enabled with GP_PTP_CONTEXT_PROBE=1. Off by default and inert when off.
 */
int
ptp_context_probe_enabled (void)
{
	static int cached = -1;
	const char *e;

	if (cached != -1)
		return cached;
	e = getenv ("GP_PTP_CONTEXT_PROBE");
	cached = (e && strcmp (e, "0") != 0) ? 1 : 0;
	return cached;
}

/* The bindings array is __thread, so its address is a stable per-thread tag
 * that needs no pthread dependency inside the camlib. */
unsigned long
ptp_context_thread_tag (void)
{
	return (unsigned long)(uintptr_t)&bindings[0];
}

/* Identity-only probe. Safe wherever ownership is uncertain: it never touches
 * the context object, so it cannot fault on a freed one and cannot observe a
 * value that was never written. */
static void
ptp_probe_line (const char *where, void *owner, GPContext *context,
		unsigned long detail)
{
	struct timespec ts;
	static unsigned long seq;

	clock_gettime (CLOCK_MONOTONIC, &ts);
	fprintf (stderr, "[ctx-probe] t=%ld.%03ld seq=%lu thr=%lx %s "
		 "owner=%p gen=%u ctx=%p detail=%#lx",
		 (long)ts.tv_sec, (long)(ts.tv_nsec / 1000000),
		 ++seq, ptp_context_thread_tag (), where, owner,
		 owner ? ((PTPData *)owner)->context_generation : 0u,
		 (void *)context, detail);
}

void
ptp_context_probe (const char *where, void *owner, GPContext *context,
		   unsigned long detail)
{
	if (!ptp_context_probe_enabled ())
		return;
	ptp_probe_line (where, owner, context, detail);
	fprintf (stderr, "\n");
	fflush (stderr);
}

/* Field-reporting probe. Only call this where the context is demonstrably
 * owned -- here, immediately after this module has taken a reference. The
 * reference count comes from the core accessor, not from a copied struct
 * layout, so it cannot silently disagree with the real definition. */
void
ptp_context_probe_owned (const char *where, void *owner, GPContext *context,
			 unsigned long detail)
{
	if (!ptp_context_probe_enabled ())
		return;
	ptp_probe_line (where, owner, context, detail);
	if (context)
		fprintf (stderr, " ref=%u", gp_context_ref_count (context));
	fprintf (stderr, "\n");
	fflush (stderr);
}

void
ptp_context_owner_init (void *owner)
{
	PTPData *data = owner;
	uint32_t generation;

	if (!data)
		return;
	/* 32-bit atomic increment is supported by the Polaris ARM EABI toolchain.
	 * Generation zero is reserved for fixtures/uninitialised owners. */
	do {
		generation = __sync_add_and_fetch (&next_context_generation, 1);
	} while (!generation);
	data->context_generation = generation;
}

/* Replace the contents of one binding slot, keeping the stored context alive
 * for exactly as long as the slot refers to it (issue #190).
 *
 * Callers (the operation entry points) pass a BORROWED context: the closed
 * Polaris app creates one GPContext at startup and frees it from its camera
 * teardown, which runs on app-initiated resets and on every failed
 * re-initialisation, while an operation started by an earlier attempt may
 * still be inside the USB transport using the bound pointer. Nothing else in
 * the process ever calls gp_context_ref, so without this reference the
 * transport's next call through context->progress_start_func lands in freed
 * heap (confirmed on hardware: identical lr/garbage-callee with a forced
 * mid-download teardown; see benro-polaris-firmware-patcher#190).
 *
 * What this does and does not serialise. The reference is taken while the
 * caller still guarantees the object is live, and from that moment the app's
 * own release cannot free it while any binding refers to it. The count is
 * atomic in the core, so our increment cannot race the app's decrement into a
 * wrong value. What no lock inside this camlib can order is the app's release
 * itself -- it statically links its own gp_context_unref and touches the count
 * directly -- so the residual window is a release that lands *before* this
 * reference is taken. That window is exactly the fault we reproduced; closing
 * it fully needs the app to stop freeing while an operation is in flight, which
 * is an integration-side change outside this repository. Everything after the
 * reference is taken is safe, and every path below is balanced.
 *
 * The ledger (ptp_context_held_refs) counts references this module currently
 * holds. It exists so the balance is observable from a test without reaching
 * into an opaque object; it is not used for behaviour. */
static unsigned int held_refs;

unsigned int
ptp_context_held_refs (void)
{
	return __atomic_load_n(&held_refs, __ATOMIC_ACQUIRE);
}

static void
ptp_context_replace (unsigned int slot, void *owner, uint32_t generation,
		     GPContext *context)
{
	GPContext *previous = bindings[slot].context;

	if (previous && previous != context) {
		gp_context_unref (previous);
		__atomic_sub_fetch(&held_refs, 1, __ATOMIC_ACQ_REL);
	}
	if (context && context != previous) {
		gp_context_ref (context);
		__atomic_add_fetch(&held_refs, 1, __ATOMIC_ACQ_REL);
		ptp_context_probe_owned ("bind-held", owner, context, generation);
	}
	bindings[slot].owner = owner;
	bindings[slot].generation = generation;
	bindings[slot].context = context;
}

void
ptp_context_set (void *owner, GPContext *context)
{
	unsigned int i, free_slot = PTP_CONTEXT_BINDINGS;
	uint32_t generation;

	if (!owner)
		return;
	generation = ((PTPData *)owner)->context_generation;
	if (!generation)
		return;
	ptp_context_probe (context ? "bind-set" : "bind-clear",
			   owner, context, 0);
	for (i = 0; i < PTP_CONTEXT_BINDINGS; i++) {
		if (bindings[i].owner == owner &&
		    bindings[i].generation == generation) {
			if (!context)
				ptp_context_replace (i, NULL, 0, NULL);
			else
				ptp_context_replace (i, owner, generation,
						     context);
			return;
		}
		/* The same address with a different generation is a new owner
		 * lifetime. Reclaim this thread's obsolete slot instead of allowing
		 * repeated camera reconnects to exhaust the fixed TLS table. */
		if (bindings[i].owner == owner)
			ptp_context_replace (i, NULL, 0, NULL);
		if (!bindings[i].owner && free_slot == PTP_CONTEXT_BINDINGS)
			free_slot = i;
	}
	if (!context)
		return;
	if (free_slot == PTP_CONTEXT_BINDINGS) {
		/* Losing UI callbacks is safer than exposing an operation to a context
		 * owned by another thread. */
		fprintf (stderr, "ptp2: per-thread operation-context table full\n");
		return;
	}
	ptp_context_replace (free_slot, owner, generation, context);
}

GPContext *
ptp_context_get (void *owner)
{
	unsigned int i;
	uint32_t generation;

	if (!owner)
		return NULL;
	generation = ((PTPData *)owner)->context_generation;
	if (!generation)
		return NULL;
	for (i = 0; i < PTP_CONTEXT_BINDINGS; i++)
		if (bindings[i].owner == owner &&
		    bindings[i].generation == generation)
			return bindings[i].context;
	return NULL;
}

/* Release every binding belonging to the calling thread.
 *
 * The table is thread-local, so this can only reach the calling thread's own
 * entries -- it is not a global drain and does not claim to be. A caller that
 * owns a thread's whole lifetime (an operation thread finishing, a wrapper
 * tearing down its worker) calls this on the way out so a reference cannot be
 * stranded when the thread goes away. Threads that clear explicitly through
 * ptp_context_set(owner, NULL) do not need it. */
void
ptp_context_release_thread (void)
{
	unsigned int i;

	for (i = 0; i < PTP_CONTEXT_BINDINGS; i++)
		if (bindings[i].owner)
			ptp_context_replace (i, NULL, 0, NULL);
}
