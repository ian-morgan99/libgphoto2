#include "config.h"

#include <stdio.h>
#include <gphoto2/gphoto2-library.h>

#include "ptp-context.h"

#define PTP_CONTEXT_BINDINGS 64

typedef struct {
	void *owner;
	GPContext *context;
} PTPContextBinding;

/* This module is part of ptp2.so, so the same TLS table is shared by the
 * operation entry points and the USB transport callbacks in that module. */
static __thread PTPContextBinding bindings[PTP_CONTEXT_BINDINGS];

void
ptp_context_set (void *owner, GPContext *context)
{
	unsigned int i, free_slot = PTP_CONTEXT_BINDINGS;

	if (!owner)
		return;
	for (i = 0; i < PTP_CONTEXT_BINDINGS; i++) {
		if (bindings[i].owner == owner) {
			if (!context) {
				bindings[i].owner = NULL;
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
	bindings[free_slot].context = context;
}

GPContext *
ptp_context_get (void *owner)
{
	unsigned int i;

	if (!owner)
		return NULL;
	for (i = 0; i < PTP_CONTEXT_BINDINGS; i++)
		if (bindings[i].owner == owner)
			return bindings[i].context;
	return NULL;
}
