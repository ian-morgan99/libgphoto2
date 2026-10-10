#include "pentax-bulb-lifecycle.h"

#include <string.h>

int
pentax_bulb_run_lifecycle (const PentaxBulbLifecycleOps *ops,
	PentaxBulbLifecycleState *state)
{
	int result;

	if (!ops || !state || !ops->set_edge || !ops->wait || !ops->collect)
		return GP_ERROR_BAD_PARAMETERS;

	memset (state, 0, sizeof (*state));
	state->explicit_stop_result = GP_OK;
	state->cleanup_stop_result = GP_OK;
	state->collection_result = GP_OK;

	if (ops->check_cancel) {
		result = ops->check_cancel (ops->opaque);
		if (result < GP_OK)
			return result;
	}

	state->start_attempted = 1;
	result = ops->set_edge (ops->opaque, 1);
	if (result < GP_OK) {
		state->operator_intervention_required = 1;
		return result;
	}
	state->start_confirmed = 1;
	state->shutter_open = 1;

	if (ops->check_cancel)
		result = ops->check_cancel (ops->opaque);
	else
		result = GP_OK;
	if (result >= GP_OK)
		result = ops->wait (ops->opaque);
	if (result >= GP_OK && ops->check_cancel)
		result = ops->check_cancel (ops->opaque);

	if (result >= GP_OK) {
		state->explicit_stop_attempted = 1;
		state->explicit_stop_result = ops->set_edge (ops->opaque, 0);
		result = state->explicit_stop_result;
		if (result >= GP_OK)
			state->shutter_open = 0;
	}

	if (state->shutter_open) {
		state->cleanup_stop_attempted = 1;
		state->cleanup_stop_result = ops->set_edge (ops->opaque, 0);
		if (state->cleanup_stop_result >= GP_OK)
			state->shutter_open = 0;
		else
			state->operator_intervention_required = 1;
	}

	if (!state->shutter_open) {
		state->collection_attempted = 1;
		state->collection_result = ops->collect (ops->opaque);
		if (result >= GP_OK && state->collection_result < GP_OK)
			result = state->collection_result;
	}

	return result;
}
