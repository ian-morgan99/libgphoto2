#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gphoto2/gphoto2-context.h>

#include "../camlibs/ptp2/ptp.h"
#include "../camlibs/ptp2/ptp-context.h"
#include "../camlibs/ptp2/ptp-private.h"

/* Regression for benro-polaris-firmware-patcher#190: the closed Polaris app
 * owns exactly one GPContext and frees it from its camera teardown while an
 * operation started by an earlier attempt may still be inside the USB transport
 * using the bound pointer. The binding must keep the context alive across that
 * free.
 *
 * The reference count is read through gp_context_ref_count(), the core's own
 * accessor. Nothing here mirrors the layout of an opaque GPContext, so these
 * assertions cannot silently survive a change to the real struct.
 *
 * Every check is fail-fast. Continuing past a failed retention assertion would
 * read an object the owner's unref may already have freed, which turns the
 * witness into the bug. Barriers and condition variables only: no timing sleeps.
 */

static PTPData owner;
static GPContext *main_context;
static GPContext *worker_context;

static pthread_mutex_t lifecycle_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t lifecycle_cond = PTHREAD_COND_INITIALIZER;
static int worker_bound;
static int owner_recreated;

/* Teardown-race choreography. The worker is the in-flight operation, the main
 * thread is the app's camera teardown. */
static pthread_mutex_t race_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t race_cond = PTHREAD_COND_INITIALIZER;
static int race_bound;
static int race_torndown;

static void
fail (const char *what, unsigned long got, unsigned long want)
{
	printf ("%s (got %lu, want %lu)\n", what, got, want);
	exit (1);
}

static void
expect_ref (GPContext *context, unsigned int want)
{
	unsigned int got = gp_context_ref_count (context);

	if (got != want)
		fail ("unexpected refcount", got, want);
}

static void
expect_held (unsigned int want)
{
	unsigned int got = ptp_context_held_refs ();

	if (got != want)
		fail ("references held by the binding table", got, want);
}

static void *
worker (void *unused)
{
	(void)unused;
	ptp_context_set (&owner, worker_context);
	if (ptp_context_get (&owner) != worker_context)
		fail ("worker could not resolve its own binding", 0, 1);
	ptp_context_set (&owner, NULL);
	return NULL;
}

/* The competing sequence: the owner's release lands while the operation still
 * holds the binding. Before the fix the object was freed under the operation
 * and the use below was a use-after-free. */
static void *
teardown_race_worker (void *unused)
{
	(void)unused;

	ptp_context_set (&owner, main_context);		/* operation acquires */
	if (ptp_context_get (&owner) != main_context)
		fail ("racing operation lost its binding", 0, 1);

	pthread_mutex_lock (&race_mutex);
	race_bound = 1;
	pthread_cond_broadcast (&race_cond);
	while (!race_torndown)
		pthread_cond_wait (&race_cond, &race_mutex);
	pthread_mutex_unlock (&race_mutex);

	/* The app has dropped its only reference while this operation was in
	 * flight. The object must still be alive, and this is the call the
	 * transport makes through the bound pointer. */
	if (ptp_context_get (&owner) != main_context)
		fail ("binding vanished during teardown", 0, 1);
	expect_ref (main_context, 1);			/* held by the binding */
	gp_context_ref (main_context);
	gp_context_unref (main_context);

	/* Draining: the operation finishes and releases. */
	ptp_context_set (&owner, NULL);
	if (ptp_context_get (&owner) != NULL)
		fail ("binding survived its own clear", 0, 1);
	return NULL;
}

/* Reconnect against a reused owner address: the stale generation must not
 * resolve, and its slot must be reclaimed rather than accumulated. */
static void *
lifecycle_worker (void *unused)
{
	(void)unused;
	ptp_context_set (&owner, worker_context);
	if (ptp_context_get (&owner) != worker_context)
		fail ("worker could not resolve its own binding", 0, 1);
	pthread_mutex_lock (&lifecycle_mutex);
	worker_bound = 1;
	pthread_cond_broadcast (&lifecycle_cond);
	while (!owner_recreated)
		pthread_cond_wait (&lifecycle_cond, &lifecycle_mutex);
	pthread_mutex_unlock (&lifecycle_mutex);
	if (ptp_context_get (&owner) != NULL)
		fail ("stale generation resolved against a new owner lifetime", 1, 0);
	/* Reclaim this thread's stale-generation slot, which also releases the
	 * reference it held, so the reclaim path is exercised and nothing leaks. */
	ptp_context_set (&owner, worker_context);
	ptp_context_set (&owner, NULL);
	return NULL;
}

/* A thread finishing its work releases through the thread-exit entry point
 * rather than an explicit per-owner clear, and must not strand a reference. */
static void *
releasing_worker (void *unused)
{
	(void)unused;
	ptp_context_set (&owner, worker_context);
	if (ptp_context_get (&owner) != worker_context)
		fail ("worker could not resolve its own binding", 0, 1);
	ptp_context_release_thread ();
	return NULL;
}

static pthread_t
start (void *(*fn)(void *))
{
	pthread_t thread;

	if (pthread_create (&thread, NULL, fn, NULL) != 0)
		exit (2);
	return thread;
}

static void
finish (pthread_t thread)
{
	if (pthread_join (thread, NULL) != 0)
		exit (2);
}

int
main (void)
{
	pthread_t thread;
	int i;

	memset (&owner, 0, sizeof (owner));
	ptp_context_owner_init (&owner);
	main_context = gp_context_new ();
	worker_context = gp_context_new ();
	if (!main_context || !worker_context)
		return 1;
	expect_held (0);

	/* --- thread isolation and clear ------------------------------------- */
	ptp_context_set (&owner, main_context);
	thread = start (worker);
	finish (thread);
	if (ptp_context_get (&owner) != main_context)
		fail ("main binding was disturbed by another thread", 0, 1);
	expect_held (1);
	ptp_context_set (&owner, NULL);
	if (ptp_context_get (&owner) != NULL)
		fail ("binding survived clear", 0, 1);
	expect_held (0);

	/* --- repeated init retry, error/cancel exit ------------------------- */
	for (i = 0; i < 50; i++) {
		ptp_context_set (&owner, main_context);
		expect_ref (main_context, 2);
		ptp_context_set (&owner, NULL);		/* error/cancel exit */
		expect_ref (main_context, 1);
		expect_held (0);
	}

	/* --- rebinding replaces rather than accumulates --------------------- */
	ptp_context_set (&owner, main_context);
	ptp_context_set (&owner, main_context);		/* same context twice */
	expect_ref (main_context, 2);
	expect_held (1);
	ptp_context_set (&owner, worker_context);	/* switch context */
	expect_ref (main_context, 1);
	expect_ref (worker_context, 2);
	expect_held (1);
	ptp_context_set (&owner, NULL);
	expect_held (0);

	/* --- the competing teardown ----------------------------------------- */
	pthread_mutex_lock (&race_mutex);
	thread = start (teardown_race_worker);
	while (!race_bound)
		pthread_cond_wait (&race_cond, &race_mutex);
	expect_ref (main_context, 2);			/* owner + binding */
	pthread_mutex_unlock (&race_mutex);

	gp_context_unref (main_context);		/* the app's camera teardown */

	pthread_mutex_lock (&race_mutex);
	race_torndown = 1;
	pthread_cond_broadcast (&race_cond);
	pthread_mutex_unlock (&race_mutex);
	finish (thread);
	/* The worker asserted the object was still usable and then drained. Its
	 * release took the count to zero, so main_context is now freed. */
	expect_held (0);
	main_context = gp_context_new ();		/* fresh owner reference */
	if (!main_context)
		return 1;

	/* --- reconnect against a reused owner address ----------------------- */
	thread = start (lifecycle_worker);
	pthread_mutex_lock (&lifecycle_mutex);
	while (!worker_bound)
		pthread_cond_wait (&lifecycle_cond, &lifecycle_mutex);
	ptp_context_owner_init (&owner);		/* allocator address reuse */
	ptp_context_set (&owner, main_context);		/* new lifetime rebinds */
	if (ptp_context_get (&owner) != main_context)
		fail ("new lifetime could not bind", 0, 1);
	owner_recreated = 1;
	pthread_cond_broadcast (&lifecycle_cond);
	pthread_mutex_unlock (&lifecycle_mutex);
	finish (thread);
	ptp_context_set (&owner, NULL);
	expect_held (0);

	/* --- thread exit releases whatever the thread held ------------------ */
	thread = start (releasing_worker);
	finish (thread);
	expect_held (0);
	if (ptp_context_get (&owner) != NULL)
		fail ("binding survived thread release", 0, 1);

	/* The app drops its own references; the binding table has already given
	 * back everything it took. */
	gp_context_unref (main_context);
	gp_context_unref (worker_context);
	puts ("test-ptp-context: thread isolation, reused-owner generations, competing teardown and reference balance pass");
	return 0;
}
