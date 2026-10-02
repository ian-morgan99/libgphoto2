/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2001-2005 Mariusz Woloszyn <emsi@ipartners.pl>
 * Copyright (C) 2003-2026 Marcus Meissner <marcus@jet.franken.de>
 * Copyright (C) 2005 Hubert Figuiere <hfiguiere@teaser.fr>
 * Copyright (C) 2009-2024 Axel Waggershauser <awagger@web.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ptp.h"
#include "ptp-bugs.h"
#include "ptp-private.h"

#include "pentax-utils.h"

/* Minimum size for Pentax conditions block. */
#define PENTAX_CONDITIONS_MIN_SIZE 528

/* Check if Pentax admission probe is OK. */
int
pentax_admission_probe_ok (const unsigned char *data, size_t size,
	PentaxAdmissionPolicy policy)
{
	return pentax_admission_block_reason (data, size, policy) ==
		PENTAX_ADMISSION_BLOCK_NONE;
}

/* Get the name of a Pentax admission block reason. */
const char *
pentax_admission_block_reason_name (PentaxAdmissionBlockReason reason)
{
	switch (reason) {
	case PENTAX_ADMISSION_BLOCK_NONE:
		return "none";
	case PENTAX_ADMISSION_BLOCK_UNREADABLE:
		return "conditions-unreadable";
	case PENTAX_ADMISSION_BLOCK_UNSAFE_ACTIVITY:
		return "unsafe-activity";
	case PENTAX_ADMISSION_BLOCK_TRANSFER_CANDIDATE_AVAILABLE:
		return "transfer-candidate-available";
	case PENTAX_ADMISSION_BLOCK_SELECTOR_PRESENT:
		return "selector-present";
	case PENTAX_ADMISSION_BLOCK_OUTPUT_UNRESOLVED:
		return "output-obligation-unresolved";
	}
	return "unknown";
}

/* Pentax admission block reason. */
PentaxAdmissionBlockReason
pentax_admission_block_reason (const unsigned char *data, size_t size,
	PentaxAdmissionPolicy policy)
{
	if (!data || (size < PENTAX_CONDITIONS_MIN_SIZE))
		return PENTAX_ADMISSION_BLOCK_UNREADABLE;
	if (policy == PENTAX_ADMISSION_STRICT &&
	    (pentax_get_u32le (data + 104) & PENTAX_CONDITION_ACTIVITY_UNSAFE))
		return PENTAX_ADMISSION_BLOCK_UNSAFE_ACTIVITY;
	if (pentax_get_u32le (data + 32) == 1)
		return PENTAX_ADMISSION_BLOCK_TRANSFER_CANDIDATE_AVAILABLE;
	if (pentax_get_u32le (data + 36) != 0)
		return PENTAX_ADMISSION_BLOCK_SELECTOR_PRESENT;
	return PENTAX_ADMISSION_BLOCK_NONE;
}

/* Pentax capture output obligation resolved. */
int
pentax_capture_output_obligation_resolved (int capture_accepted,
	int primary_published, int all_expected_outputs_published)
{
	return !capture_accepted ||
		(primary_published && all_expected_outputs_published);
}

/* Pentax candidate output published. */
int
pentax_candidate_output_published (int transfer_succeeded, int filename_known,
	int filesystem_publication_succeeded)
{
	return transfer_succeeded && filename_known &&
		filesystem_publication_succeeded;
}

/* Pentax capture initiate response ambiguous. */
int
pentax_capture_initiate_response_ambiguous (uint16_t response)
{
	/* ptp.h reserves 0x02f9..0x02ff for transport/session failures. Unlike an
	 * explicit PTP response, these do not prove whether the camera accepted the
	 * command before the response path failed. */
	return response >= 0x02f9 && response <= 0x02ff;
}

/* Pentax capture output contract known. */
int
pentax_capture_output_contract_known (const unsigned char *data, size_t size)
{
	uint32_t format;

	/* IMAGE Transmitter 2 documents 0=JPEG, 1=RAW, 2=RAW+JPEG, 3=TIFF.
	 * A short/unknown value must not silently turn a multi-file capture into a
	 * single-file obligation. */
	if (!data || size < 528)
		return 0;
	format = pentax_get_u32le (data + 524);
	return format <= 3;
}

/* Pentax expected extra candidates. */
unsigned int
pentax_expected_extra_candidates (const unsigned char *data, size_t size)
{
	return pentax_get_u32le (data + 524) == 2 ? 1U : 0U;
}

/* Pentax stale candidate baseline. */
uint32_t
pentax_stale_candidate_baseline (const unsigned char *data, size_t size)
{
	if (!data || (size < PENTAX_CONDITIONS_MIN_SIZE))
		return 0;
	if (pentax_get_u32le (data + 32) != 1)
		return 0;
	return pentax_get_u32le (data + 36);
}

/* Pentax reconcile conditions. */
PentaxReconcileDecision
pentax_reconcile_conditions (const unsigned char *data, size_t size, uint32_t *candidate_out)
{
	if (!data || (size < PENTAX_CONDITIONS_MIN_SIZE))
		return PENTAX_RECONCILE_UNREADABLE;
	uint32_t activity = pentax_get_u32le (data + 104);
	uint32_t candidate = pentax_get_u32le (data + 36);
	if (activity & PENTAX_CONDITION_ACTIVITY_UNSAFE)
		return PENTAX_RECONCILE_UNSAFE;
	if (candidate) {
		if (candidate_out)
			*candidate_out = candidate;
		return PENTAX_RECONCILE_STALE_CANDIDATE;
	}
	return PENTAX_RECONCILE_IDLE;
}

/* Pentax camera readiness. */
PentaxReadiness
pentax_camera_readiness (const unsigned char *data, unsigned int size)
{
	if ((data == NULL) || (size < PENTAX_CONDITIONS_MIN_SIZE))
		return PENTAX_READINESS_UNKNOWN;
	{
		uint32_t activity = pentax_get_u32le (data + 104);
		uint32_t candidate = pentax_get_u32le (data + 36);
		if ((activity & PENTAX_CONDITION_ACTIVITY_UNSAFE) == 0 && !candidate)
			return PENTAX_READINESS_IDLE;
		return PENTAX_READINESS_BUSY;
	}
}

/* Pentax conditions in astro mode. */
int
pentax_conditions_in_astro_mode (const PentaxConditions *conditions)
{
	/* Whether the camera is currently in Astro Tracer mode, per IT2's own signal:
	 * the exposure-mode dial value (offset 184) equals ExpMode.AstroTracer (20).
	 * This is the authoritative "in astro mode" test — the offset-504 capability
	 * bit (PENTAX_CONDITION_ASTROTRACER3) only says the body CAN do it, and the
	 * offset-320 status bits describe shift/aperture sub-states, not the mode.
	 * Model-agnostic, so it covers the K-1 II as well as the K-3 III. */
	return conditions->exposure_mode == 20;
}

/* Pentax capture timeout ms. */
unsigned int
pentax_capture_timeout_ms (const PentaxConditions *conditions)
{
	unsigned int base = PENTAX_CAPTURE_TIMEOUT_MS_BASE;
	unsigned int margin = PENTAX_CAPTURE_PROCESSING_MARGIN_MS;

	if (conditions->activity_flags & PENTAX_CONDITION_ACTIVITY_UNSAFE)
		return 0;

	if (conditions->bulb_timer_seconds)
		return (conditions->bulb_timer_seconds * 1000) +
			(conditions->bulb_timer_denominator ?
			 (60000 / conditions->bulb_timer_denominator) : 0) +
			margin;

	if (conditions->exposure_mode == 20) /* AstroTracer */
		return base + margin;

	if (conditions->exposure_mode == 21) /* Pixel shift */
		return (base * PENTAX_PIXEL_SHIFT_MULTIPLIER) + margin;

	return base;
}

/* Pentax exposure phase ms. */
unsigned int
pentax_exposure_phase_ms (const PentaxConditions *conditions)
{
	if (conditions->activity_flags & PENTAX_CONDITION_ACTIVITY_UNSAFE)
		return 0;

	if (conditions->bulb_timer_seconds)
		return (conditions->bulb_timer_seconds * 1000) +
			(conditions->bulb_timer_denominator ?
			 (60000 / conditions->bulb_timer_denominator) : 0);

	if (conditions->exposure_mode == 20) /* AstroTracer */
		return 0; /* Handled by capture timeout */

	if (conditions->exposure_mode == 21) /* Pixel shift */
		return 0; /* Handled by capture timeout */

	return 0;
}

/* Pentax capture needs idle wait. */
int
pentax_capture_needs_idle_wait (const PentaxConditions *conditions)
{
	/* Issue #122 (TA follow-up): fail-closed post-capture readiness wait.  Polls
	 * until the camera reports idle via the activity flags. */
	return (conditions->activity_flags & PENTAX_CONDITION_ACTIVITY_UNSAFE) != 0;
}

/* Pentax recovery probe can clear. */
int
pentax_recovery_probe_can_clear (PentaxAdmissionBlockReason reason,
	int capture_output_pending)
{
	/* Readiness and output completion are independent predicates. A clean
	 * conditions frame may clear a stale activity barrier, but never an
	 * obligation created by an accepted capture and not yet published. */
	return (reason == PENTAX_ADMISSION_BLOCK_NONE) &&
		!capture_output_pending;
}

/* Pentax admission recovery action. */
const char *
pentax_admission_recovery_action (PentaxAdmissionBlockReason reason)
{
	switch (reason) {
	case PENTAX_ADMISSION_BLOCK_NONE:
		return "";
	case PENTAX_ADMISSION_BLOCK_UNREADABLE:
		return "re-probe-before-retry";
	case PENTAX_ADMISSION_BLOCK_UNSAFE_ACTIVITY:
		return "hold-shutter; wait-for-idle";
	case PENTAX_ADMISSION_BLOCK_TRANSFER_CANDIDATE_AVAILABLE:
		return "hold-shutter; inspect-transfer-candidate";
	case PENTAX_ADMISSION_BLOCK_SELECTOR_PRESENT:
		return "hold-shutter; recover-output-with-ownership";
	case PENTAX_ADMISSION_BLOCK_OUTPUT_UNRESOLVED:
		return "hold-shutter; inspect-diagnostics";
	}
	return "unknown";
}
