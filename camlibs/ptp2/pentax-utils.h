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

#ifndef PENTAX_UTILS_H
#define PENTAX_UTILS_H

#include <stdint.h>

/* Pentax condition flags for activity states that make mutating vendor
 * operations unsafe after a reconnect (issue #33). */
#define PENTAX_CONDITION_ACTIVITY_SHOOTING       0x00000001U
#define PENTAX_CONDITION_ACTIVITY_PROCESSING     0x00000002U
#define PENTAX_CONDITION_ACTIVITY_MOVIE_MODE     0x00000100U
#define PENTAX_CONDITION_ACTIVITY_MOVIE_RECORDING 0x00000200U
#define PENTAX_CONDITION_ACTIVITY_MIRROR_UP_MODE 0x00000400U
#define PENTAX_CONDITION_ACTIVITY_MIRROR_UPPING  0x00000800U
#define PENTAX_CONDITION_ACTIVITY_INTERVAL_MODE  0x00001000U
#define PENTAX_CONDITION_ACTIVITY_MULTI_MODE     0x00004000U
#define PENTAX_CONDITION_ACTIVITY_MULTI_CAPTURE  0x00008000U
#define PENTAX_CONDITION_ACTIVITY_SELF_TIMER     0x00100000U

/* Activity states that make mutating vendor operations unsafe after a
 * reconnect (issue #33). Uses named constants rather than literal bit 0. */
#define PENTAX_CONDITION_ACTIVITY_UNSAFE ( \
	PENTAX_CONDITION_ACTIVITY_SHOOTING       | \
	PENTAX_CONDITION_ACTIVITY_PROCESSING     | \
	PENTAX_CONDITION_ACTIVITY_MOVIE_RECORDING | \
	PENTAX_CONDITION_ACTIVITY_MIRROR_UPPING  | \
	PENTAX_CONDITION_ACTIVITY_INTERVAL_MODE  | \
	PENTAX_CONDITION_ACTIVITY_MULTI_CAPTURE  | \
	PENTAX_CONDITION_ACTIVITY_SELF_TIMER)

/* Pentax admission policies. */
typedef enum {
	PENTAX_ADMISSION_STRICT = 0,
	PENTAX_ADMISSION_OUTPUT_SAFE = 1
} PentaxAdmissionPolicy;

/* Pentax admission block reasons. */
typedef enum {
	PENTAX_ADMISSION_BLOCK_NONE = 0,
	PENTAX_ADMISSION_BLOCK_UNREADABLE,
	PENTAX_ADMISSION_BLOCK_UNSAFE_ACTIVITY,
	PENTAX_ADMISSION_BLOCK_TRANSFER_CANDIDATE_AVAILABLE,  // +32
	PENTAX_ADMISSION_BLOCK_SELECTOR_PRESENT,              // +36
	PENTAX_ADMISSION_BLOCK_OUTPUT_UNRESOLVED
} PentaxAdmissionBlockReason;

/* Pentax reconcile decisions. */
typedef enum {
	PENTAX_RECONCILE_IDLE = 0,
	PENTAX_RECONCILE_STALE_CANDIDATE,
	PENTAX_RECONCILE_UNSAFE,
	PENTAX_RECONCILE_UNREADABLE
} PentaxReconcileDecision;

/* Pentax capture timeout states. */
typedef enum {
	PENTAX_CAPTURE_TIMEOUT_STATE_UNKNOWN = 0,
	PENTAX_CAPTURE_TIMEOUT_STATE_EXPOSING,
	PENTAX_CAPTURE_TIMEOUT_STATE_PROCESSING
} PentaxCaptureTimeoutState;

/* Pentax readiness states. */
typedef enum {
	PENTAX_READINESS_UNKNOWN = 0,
	PENTAX_READINESS_IDLE,
	PENTAX_READINESS_BUSY
} PentaxReadiness;

/* Pentax condition structure. */
typedef struct {
	uint8_t operation_state;
	uint32_t activity_flags;
	uint32_t exposure_mode;
	uint32_t user_mode;
	uint32_t exposure_step;
	uint32_t bulb_timer_seconds;
	uint32_t bulb_timer_denominator;
	uint32_t aperture_numerator;
	uint32_t aperture_denominator;
	int32_t exposure_comp_numerator;
	uint32_t exposure_comp_denominator;
	uint32_t iso;
	uint32_t open_av_num;
	uint32_t astro_status_flags;
	uint32_t drive_mode;
	uint32_t white_balance;
	uint32_t af_mode;
	uint32_t capability_flags;
	uint32_t astro_limit_seconds;
	int has_astro_limit;
} PentaxConditions;

/* Pentax live view geometry. */
typedef struct {
	uint16_t area_width;
	uint16_t area_height;
	uint16_t active_width;
	uint16_t active_height;
	uint16_t contrast_af_active_width;
	uint16_t contrast_af_active_height;
	uint16_t contrast_af_spot_width;
	uint16_t contrast_af_spot_height;
} PentaxLiveViewGeometry;

/* Get a 32-bit little-endian value from unaligned memory. */
static inline uint32_t
pentax_get_u32le (const unsigned char *data);

/* Get a 16-bit little-endian value from unaligned memory. */
static inline uint16_t
pentax_get_u16le (const unsigned char *data);

/* Check if Pentax admission probe is OK. */
int
pentax_admission_probe_ok (const unsigned char *data, size_t size,
	PentaxAdmissionPolicy policy);

/* Get the name of a Pentax admission block reason. */
const char *
pentax_admission_block_reason_name (PentaxAdmissionBlockReason reason);

/* Pentax admission block reason. */
PentaxAdmissionBlockReason
pentax_admission_block_reason (const unsigned char *data, size_t size,
	PentaxAdmissionPolicy policy);

/* Pentax capture output obligation resolved. */
int
pentax_capture_output_obligation_resolved (int capture_accepted,
	int primary_published, int all_expected_outputs_published);

/* Pentax candidate output published. */
int
pentax_candidate_output_published (int transfer_succeeded, int filename_known,
	int filesystem_publication_succeeded);

/* Pentax capture initiate response ambiguous. */
int
pentax_capture_initiate_response_ambiguous (uint16_t response);

/* Pentax capture output contract known. */
int
pentax_capture_output_contract_known (const unsigned char *data, size_t size);

/* Pentax expected extra candidates. */
unsigned int
pentax_expected_extra_candidates (const unsigned char *data, size_t size);

/* Pentax stale candidate baseline. */
uint32_t
pentax_stale_candidate_baseline (const unsigned char *data, size_t size);

/* Pentax reconcile conditions. */
PentaxReconcileDecision
pentax_reconcile_conditions (const unsigned char *data, size_t size, uint32_t *candidate_out);

/* Pentax camera readiness. */
PentaxReadiness
pentax_camera_readiness (const unsigned char *data, unsigned int size);

/* Pentax conditions in astro mode. */
int
pentax_conditions_in_astro_mode (const PentaxConditions *conditions);

/* Pentax capture timeout ms. */
unsigned int
pentax_capture_timeout_ms (const PentaxConditions *conditions);

/* Pentax exposure phase ms. */
unsigned int
pentax_exposure_phase_ms (const PentaxConditions *conditions);

/* Pentax capture needs idle wait. */
int
pentax_capture_needs_idle_wait (const PentaxConditions *conditions);

/* Pentax recovery probe can clear. */
int
pentax_recovery_probe_can_clear (PentaxAdmissionBlockReason reason,
	int capture_output_pending);

/* Pentax admission recovery action. */
const char *
pentax_admission_recovery_action (PentaxAdmissionBlockReason reason);

#endif /* PENTAX_UTILS_H */
