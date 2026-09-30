#include "config.h"

#include <stdio.h>
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
	for (i = 0; i < PTP_CONTEXT_BINDINGS; i++) {
		if (bindings[i].owner == owner &&
		    bindings[i].generation == generation) {
			if (!context) {
				bindings[i].owner = NULL;
				bindings[i].generation = 0;
				bindings[i].context = NULL;
			} else {
				bindings[i].context = context;
			}
			return;
		}
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
	bindings[free_slot].owner = owner;
	bindings[free_slot].generation = generation;
	bindings[free_slot].context = context;
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
