#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "../camlibs/ptp2/ptp-context.h"

static int owner;
static int main_context;
static int worker_context;
static int failed;

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

int
main (void)
{
	pthread_t thread;

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
	puts ("test-ptp-context: thread-local camera contexts remain isolated");
	return 0;
}
