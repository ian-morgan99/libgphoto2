#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../camlibs/ptp2/ptp.h"
#include "../camlibs/ptp2/ptp-context.h"
#include "../camlibs/ptp2/ptp-private.h"

static PTPData owner;
static int main_context;
static int worker_context;
static int failed;
static pthread_mutex_t lifecycle_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t lifecycle_cond = PTHREAD_COND_INITIALIZER;
static int worker_bound;
static int owner_recreated;

static void *
worker (void *unused)
{
	(void)unused;
	ptp_context_set (&owner, (GPContext *)&worker_context);
	if (ptp_context_get (&owner) != (GPContext *)&worker_context)
		failed = 1;
	ptp_context_set (&owner, NULL);
	return NULL;
}

static void *
lifecycle_worker (void *unused)
{
	(void)unused;
	ptp_context_set (&owner, (GPContext *)&worker_context);
	if (ptp_context_get (&owner) != (GPContext *)&worker_context)
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
	return NULL;
}

int
main (void)
{
	pthread_t thread;

	memset (&owner, 0, sizeof (owner));
	ptp_context_owner_init (&owner);
	ptp_context_set (&owner, (GPContext *)&main_context);
	if (pthread_create (&thread, NULL, worker, NULL) != 0)
		return 1;
	if (pthread_join (thread, NULL) != 0)
		return 1;
	if (failed || ptp_context_get (&owner) != (GPContext *)&main_context)
		return 1;
	ptp_context_set (&owner, NULL);
	if (ptp_context_get (&owner) != NULL)
		return 1;
	if (pthread_create (&thread, NULL, lifecycle_worker, NULL) != 0)
		return 1;
	pthread_mutex_lock (&lifecycle_mutex);
	while (!worker_bound)
		pthread_cond_wait (&lifecycle_cond, &lifecycle_mutex);
	ptp_context_owner_init (&owner); /* simulate allocator address reuse */
	owner_recreated = 1;
	pthread_cond_broadcast (&lifecycle_cond);
	pthread_mutex_unlock (&lifecycle_mutex);
	if (pthread_join (thread, NULL) != 0 || failed)
		return 1;
	puts ("test-ptp-context: thread isolation and reused-owner generations pass");
	return 0;
}
