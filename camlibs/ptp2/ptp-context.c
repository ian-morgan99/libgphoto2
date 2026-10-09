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
 * looking for. Reading the fields of a freed context is safe (the heap page is
 * still mapped); it is only calling through the pointer that faults.
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

/* GPContext is opaque outside libgphoto2 core, and the app links its own copy
 * of the same layout. Read the fields by raw offset rather than through the
 * type: reading a freed-but-mapped heap chunk is safe, and it is the only way
 * to observe the value that is about to be called through. Offsets verified
 * twice over: against struct _GPContext in libgphoto2/gphoto2-context.c, and
 * against the app's own gp_context_set_progress_funcs, which writes
 * progress_start_func at [r3,#8] and ref_count at [r3,#64]. */
#define PROBE_OFF_PROGRESS_START	8
#define PROBE_OFF_PROGRESS_UPDATE	12
#define PROBE_OFF_PROGRESS_STOP		16
#define PROBE_OFF_PROGRESS_DATA		20
#define PROBE_OFF_REF_COUNT		64

static unsigned long
probe_field (GPContext *context, unsigned int offset)
{
	uintptr_t base = (uintptr_t)context;
	uint32_t word;

	if (!base)
		return 0;
	memcpy (&word, (const char *)base + offset, sizeof (word));
	return word;
}

void
ptp_context_probe (const char *where, void *owner, GPContext *context,
		   unsigned long detail)
{
	struct timespec ts;
	static unsigned long seq;

	if (!ptp_context_probe_enabled ())
		return;
	clock_gettime (CLOCK_MONOTONIC, &ts);
	fprintf (stderr, "[ctx-probe] t=%ld.%03ld seq=%lu thr=%lx %s "
		 "owner=%p gen=%u ctx=%p detail=%#lx",
		 (long)ts.tv_sec, (long)(ts.tv_nsec / 1000000),
		 ++seq, ptp_context_thread_tag (), where, owner,
		 owner ? ((PTPData *)owner)->context_generation : 0u,
		 (void *)context, detail);
	if (context)
		fprintf (stderr, " pstart=%#lx pupdate=%#lx pstop=%#lx "
			 "pdata=%#lx ref=%lu",
			 probe_field (context, PROBE_OFF_PROGRESS_START),
			 probe_field (context, PROBE_OFF_PROGRESS_UPDATE),
			 probe_field (context, PROBE_OFF_PROGRESS_STOP),
			 probe_field (context, PROBE_OFF_PROGRESS_DATA),
			 probe_field (context, PROBE_OFF_REF_COUNT));
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
 * The ref/unref pair is balanced on every path that stores or drops a
 * pointer, including generation reclaim and slot reuse. ref_count is not
 * atomic in the core; the app only unrefs at teardown, when no operation is
 * starting, so the increment here races only with itself in practice. */
static void
ptp_context_replace (unsigned int slot, void *owner, uint32_t generation,
		     GPContext *context)
{
	GPContext *previous = bindings[slot].context;

	if (previous && previous != context)
		gp_context_unref (previous);
	if (context && context != previous)
		gp_context_ref (context);
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
