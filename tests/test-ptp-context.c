#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gphoto2/gphoto2-context.h>

#include "../camlibs/ptp2/ptp.h"
#include "../camlibs/ptp2/ptp-context.h"
#include "../camlibs/ptp2/ptp-private.h"

/* Deterministic witness for the #190 regression: the refcount is the only
 * field that proves whether the binding kept the context alive after the
 * owner dropped its reference. GPContext is opaque in the public headers, so
 * mirror its field order and let the compiler compute the offset for this
 * ABI (camlibs/ptp2/probe-offsets-check.c pins the same layout for ARM). */
struct gp_context_layout {
	void *fields[16];
	unsigned int ref_count;
};

static unsigned int
refcount_of (GPContext *context)
{
	return ((struct gp_context_layout *)context)->ref_count;
}

static PTPData owner;
static GPContext *main_context;
static GPContext *worker_context;
static int failed;
static pthread_mutex_t lifecycle_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t lifecycle_cond = PTHREAD_COND_INITIALIZER;
static int worker_bound;
static int owner_recreated;

static void *
worker (void *unused)
{
	(void)unused;
	ptp_context_set (&owner, worker_context);
	if (ptp_context_get (&owner) != worker_context)
		failed = 1;
	ptp_context_set (&owner, NULL);
	return NULL;
}

static void *
lifecycle_worker (void *unused)
{
	(void)unused;
	ptp_context_set (&owner, worker_context);
	if (ptp_context_get (&owner) != worker_context)
		failed = 1;
	pthread_mutex_lock (&lifecycle_mutex);
	worker_bound = 1;
	pthread_cond_broadcast (&lifecycle_cond);
	while (!owner_recreated)
		pthread_cond_wait (&lifecycle_cond, &lifecycle_mutex);
	pthread_mutex_unlock (&lifecycle_mutex);
	/* The owner address has been reused for a new PTPData lifetime. The
	 * worker deliberately did not clear/rebind its TLS slot: its old context
	 * must not resolve against the new lifetime. */
	if (ptp_context_get (&owner) != NULL)
		failed = 1;
	/* Reclaim this thread's stale-generation slot, which also releases the
	 * reference it held, so refcounts stay balanced and the reclaim path is
	 * exercised. */
	ptp_context_set (&owner, worker_context);
	ptp_context_set (&owner, NULL);
	return NULL;
}

int
main (void)
{
	pthread_t thread;

	memset (&owner, 0, sizeof (owner));
	ptp_context_owner_init (&owner);
	main_context = gp_context_new ();
	worker_context = gp_context_new ();
	if (!main_context || !worker_context)
		return 1;
	ptp_context_set (&owner, main_context);
	if (pthread_create (&thread, NULL, worker, NULL) != 0)
		return 1;
	if (pthread_join (thread, NULL) != 0)
		return 1;
	if (failed || ptp_context_get (&owner) != main_context)
		return 1;
	ptp_context_set (&owner, NULL);
	if (ptp_context_get (&owner) != NULL)
		return 1;

	/* Issue #190 regression: the closed Polaris app owns exactly one
	 * GPContext and frees it from its camera teardown while an operation
	 * started by an earlier attempt may still be inside the USB transport
	 * using the bound pointer (confirmed on hardware: identical fault
	 * signature with a forced mid-download teardown). The binding must keep
	 * the context alive across that free. Under ASan, a binding that held
	 * no reference makes the touches below a heap-use-after-free. */
	if (refcount_of (main_context) != 1)
		failed = 1;
	ptp_context_set (&owner, main_context);	/* binding holds a reference */
	if (refcount_of (main_context) != 2) {
		printf ("binding did not reference the context (refcount %u)\n",
			refcount_of (main_context));
		failed = 1;
	}
	gp_context_unref (main_context);	/* the app drops its only ref */
	if (ptp_context_get (&owner) != main_context)
		failed = 1;
	if (refcount_of (main_context) != 1) {
		printf ("context died with the app reference (refcount %u)\n",
			refcount_of (main_context));
		failed = 1;
	}
	/* Touch the object the transport would call through. */
	gp_context_ref (main_context);
	gp_context_unref (main_context);
	ptp_context_set (&owner, NULL);		/* binding releases: freed */
	main_context = gp_context_new ();	/* fresh context for the rest */
	if (!main_context)
		return 1;

	if (pthread_create (&thread, NULL, lifecycle_worker, NULL) != 0)
		return 1;
	pthread_mutex_lock (&lifecycle_mutex);
	while (!worker_bound)
		pthread_cond_wait (&lifecycle_cond, &lifecycle_mutex);
	ptp_context_owner_init (&owner); /* simulate allocator address reuse */
	/* Rebind the current thread too; stale generations for this address must
	 * be reclaimed rather than consuming another TLS slot per reconnect. */
	ptp_context_set (&owner, main_context);
	if (ptp_context_get (&owner) != main_context)
		failed = 1;
	owner_recreated = 1;
	pthread_cond_broadcast (&lifecycle_cond);
	pthread_mutex_unlock (&lifecycle_mutex);
	if (pthread_join (thread, NULL) != 0 || failed)
		return 1;
	ptp_context_set (&owner, NULL);
	gp_context_unref (main_context);
	gp_context_unref (worker_context);
	puts ("test-ptp-context: thread isolation, reused-owner generations and borrowed-context lifetime pass");
	return 0;
}
