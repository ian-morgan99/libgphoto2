#include <stdio.h>
#include <stdlib.h>

#include "pentax-bulb-lifecycle.h"

typedef struct {
	int edges[4];
	int edge_count;
	int start_result;
	int stop_results[2];
	int stop_calls;
	int cancel_on_check;
	int check_calls;
	int wait_result;
	int wait_calls;
	int collect_result;
	int collect_calls;
} FakeCamera;

static int
fake_set_edge (void *opaque, int start)
{
	FakeCamera *fake = opaque;
	fake->edges[fake->edge_count++] = start;
	if (start)
		return fake->start_result;
	return fake->stop_results[fake->stop_calls++];
}

static int
fake_check_cancel (void *opaque)
{
	FakeCamera *fake = opaque;
	fake->check_calls++;
	return fake->check_calls == fake->cancel_on_check ? GP_ERROR_CANCEL : GP_OK;
}

static int
fake_wait (void *opaque)
{
	FakeCamera *fake = opaque;
	fake->wait_calls++;
	return fake->wait_result;
}

static int
fake_collect (void *opaque)
{
	FakeCamera *fake = opaque;
	fake->collect_calls++;
	return fake->collect_result;
}

static void
check (int condition, const char *name)
{
	if (!condition) {
		fprintf (stderr, "FAIL: %s\n", name);
		exit (1);
	}
	printf ("PASS: %s\n", name);
}

static int
run (FakeCamera *fake, PentaxBulbLifecycleState *state)
{
	const PentaxBulbLifecycleOps ops = {
		.opaque = fake,
		.set_edge = fake_set_edge,
		.check_cancel = fake_check_cancel,
		.wait = fake_wait,
		.collect = fake_collect,
	};
	return pentax_bulb_run_lifecycle (&ops, state);
}

int
main (void)
{
	PentaxBulbLifecycleState state;
	FakeCamera fake = { .cancel_on_check = 1 };
	int result = run (&fake, &state);
	check (result == GP_ERROR_CANCEL && fake.edge_count == 0 &&
	       !state.start_attempted && !state.cleanup_stop_attempted,
	       "cancellation before start sends no shutter edge");

	fake = (FakeCamera) { .cancel_on_check = 2 };
	result = run (&fake, &state);
	check (result == GP_ERROR_CANCEL && fake.edge_count == 2 &&
	       fake.edges[0] == 1 && fake.edges[1] == 0 &&
	       state.start_confirmed && !state.shutter_open &&
	       state.cleanup_stop_attempted && !fake.wait_calls &&
	       state.collection_attempted && fake.collect_calls == 1,
	       "failure immediately after start stops and collects outputs");

	fake = (FakeCamera) { .wait_result = GP_ERROR_TIMEOUT };
	result = run (&fake, &state);
	check (result == GP_ERROR_TIMEOUT && fake.edge_count == 2 &&
	       fake.edges[0] == 1 && fake.edges[1] == 0 &&
	       state.cleanup_stop_attempted && !state.shutter_open &&
	       state.collection_attempted && fake.collect_calls == 1,
	       "wait failure stops, collects outputs, and preserves the wait error");

	fake = (FakeCamera) { .wait_result = GP_ERROR_TIMEOUT,
		.collect_result = GP_ERROR_FILE_NOT_FOUND };
	result = run (&fake, &state);
	check (result == GP_ERROR_TIMEOUT && state.collection_attempted &&
	       state.collection_result == GP_ERROR_FILE_NOT_FOUND &&
	       !state.shutter_open,
	       "output retrieval error is reported separately without masking wait failure");

	fake = (FakeCamera) { .stop_results = { GP_ERROR_IO, GP_OK } };
	result = run (&fake, &state);
	check (result == GP_ERROR_IO && fake.edge_count == 3 &&
	       fake.edges[0] == 1 && fake.edges[1] == 0 && fake.edges[2] == 0 &&
	       state.explicit_stop_attempted && state.explicit_stop_result == GP_ERROR_IO &&
	       state.cleanup_stop_attempted && state.cleanup_stop_result == GP_OK &&
	       !state.shutter_open && !state.operator_intervention_required &&
	       state.collection_attempted && fake.collect_calls == 1,
	       "failed explicit stop is retried and finalized output is collected");

	fake = (FakeCamera) { .wait_result = GP_ERROR_TIMEOUT,
		.stop_results = { GP_ERROR_IO } };
	result = run (&fake, &state);
	check (result == GP_ERROR_TIMEOUT && state.cleanup_stop_attempted &&
	       state.cleanup_stop_result == GP_ERROR_IO && state.shutter_open &&
	       state.operator_intervention_required && !fake.collect_calls,
	       "unconfirmed cleanup stop reports open/uncertain state and requires operator");

	fake = (FakeCamera) { .start_result = GP_ERROR_TIMEOUT };
	result = run (&fake, &state);
	check (result == GP_ERROR_TIMEOUT && fake.edge_count == 1 &&
	       fake.edges[0] == 1 && !state.start_confirmed &&
	       !state.explicit_stop_attempted && !state.cleanup_stop_attempted &&
	       state.operator_intervention_required && !fake.collect_calls,
	       "unconfirmed start does not send an unmatched stop or claim closure");

	fake = (FakeCamera) { .collect_result = GP_ERROR_FILE_NOT_FOUND };
	result = run (&fake, &state);
	check (result == GP_ERROR_FILE_NOT_FOUND && fake.edge_count == 2 &&
	       fake.edges[0] == 1 && fake.edges[1] == 0 &&
	       state.explicit_stop_attempted && !state.shutter_open &&
	       !state.cleanup_stop_attempted && !state.operator_intervention_required &&
	       fake.collect_calls == 1,
	       "output retrieval failure does not send a duplicate stop after confirmed close");

	fake = (FakeCamera) { 0 };
	result = run (&fake, &state);
	check (result == GP_OK && fake.edge_count == 2 &&
	       fake.edges[0] == 1 && fake.edges[1] == 0 &&
	       !state.shutter_open && !state.operator_intervention_required &&
	       fake.collect_calls == 1,
	       "successful lifecycle starts, stops, then retrieves output once");

	puts ("test-pentax-bulb-lifecycle: all tests passed");
	return 0;
}
