#ifndef CAMLIBS_PTP2_PENTAX_UTILS_H
#define CAMLIBS_PTP2_PENTAX_UTILS_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
	unsigned char *data;
	size_t size;
	size_t capacity;
	size_t offset;
} PentaxCaptureBuffer;

typedef struct {
	void *user_data;
	uint32_t max_block_size;
	int (*get_command) (void *user_data, uint8_t *operation,
		int32_t *operation_info);
	int (*get_block) (void *user_data, uint32_t requested,
		unsigned char **data, uint32_t *transferred);
	int (*is_cancelled) (void *user_data);
	int (*is_timed_out) (void *user_data);
} PentaxTransferOps;

/* IMAGE Transmitter 2 vendor-model identifiers. */
#define PENTAX_MODEL_K1_MARK_II 78400U
#define PENTAX_MODEL_K3_MARK_III 78420U
/* Reserved: IT2 treats the Monochrome as model 78420 (StartsWith match);
 * lookup currently maps it to K3_MARK_III.  Keep this ID free until a
 * hardware difference justifies a distinct entry. */
#define PENTAX_MODEL_K3_MARK_III_MONO 78421U
#define PENTAX_MODEL_KP 78380U
#define PENTAX_MODEL_K70 78370U
#define PENTAX_MODEL_K3 77760U
#define PENTAX_MODEL_K1 77970U
#define PENTAX_MODEL_GR_III 78350U
#define PENTAX_MODEL_645D 77320U
/* From IT2 MtpDevice.cs: 645Z = 77840, KF = 78520. */
#define PENTAX_MODEL_645Z 77840U
#define PENTAX_MODEL_KF 78520U
/* K-3 II (PID 0x017b) is NOT in IT2: fail-closed, vendor mode stays off.
 * Revisit only with hardware proof or a newer IT2 build that lists it. */

/* Parsed subset of the Pentax GetAllConditions response.  Mode values
 * deliberately remain raw until each value has been correlated on hardware. */
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

#define PENTAX_CONDITION_ASTRO_SHIFT_MODE        0x00000400U
#define PENTAX_CONDITION_ASTRO_APERTURE_KEEP     0x00001000U
#define PENTAX_CONDITION_ASTRO_APERTURE_INHIBIT  0x00002000U
#define PENTAX_CONDITION_ASTRO_MOVEMENT_FAILED   0x00004000U
#define PENTAX_CONDITION_ASTRO_TIME_TOO_LONG     0x00008000U

#define PENTAX_CONDITION_CAN_CHANGE_AV           0x00000001U
#define PENTAX_CONDITION_CAN_CHANGE_TV           0x00000002U
#define PENTAX_CONDITION_CAN_CHANGE_SV           0x00000004U
#define PENTAX_CONDITION_CAN_CHANGE_XV           0x00000008U
#define PENTAX_CONDITION_CAN_AUTO_SV             0x00000010U
#define PENTAX_CONDITION_TASK_CHANGING            0x00000020U
#define PENTAX_CONDITION_BULB_TIMER               0x00000040U
#define PENTAX_CONDITION_GPS_STATE_MASK           0x00000180U
#define PENTAX_CONDITION_ASTROTRACER3              0x00000200U

/* Capture-wait budget and transfer-timeout constants. Camera-reported condition
 * values are untrusted protocol input; the timeout is computed in 64-bit and
 * clamped so a corrupt value can never wrap (issue #43 regression coverage). */
#define PENTAX_CAPTURE_TIMEOUT_MS_BASE (60 * 1000)
/* Long exposures (bulb timer, astrotracer) and multi-shot composites
 * (pixel shift = 4 exposures + in-camera processing) need extra wait. */
#define PENTAX_CAPTURE_PROCESSING_MARGIN_MS (30 * 1000)
/* Pixel shift / multi-shot composites require 4x exposure time plus margin. */
#define PENTAX_PIXEL_SHIFT_MULTIPLIER 4
#define PENTAX_CAPTURE_TIMEOUT_MS_MAX (24U * 60 * 60 * 1000)
/* Fallback when conditions are unreadable after InitiateCapture: bounded so a
 * wedged camera cannot pin the caller for up to a day (review #3). */
#define PENTAX_CAPTURE_TIMEOUT_MS_FALLBACK (2U * 60 * 1000)
/* Absolute transfer ceiling and no-progress stall bound (issue #38): a stalled
 * camera trips the short bound, a legitimately huge stream trips the ceiling. */
#define PENTAX_TRANSFER_TIMEOUT_MS (30 * 60 * 1000)
#define PENTAX_TRANSFER_NOPROGRESS_TIMEOUT_MS (60 * 1000)
/* GetAllConditions payloads must carry the full parse range; offset 504
 * (capability_flags) is the final mandatory field, so >=508 bytes is the
 * single validation floor used everywhere (review #5). */
#define PENTAX_CONDITIONS_MIN_SIZE 508

/* Reconciliation decisions for a session that was already open when we
 * connected (issue #33); see pentax_reconcile_conditions(). */
typedef enum PentaxReconcileDecision {
	PENTAX_RECONCILE_IDLE = 0,		/* camera idle; capture may proceed */
	PENTAX_RECONCILE_STALE_CANDIDATE,	/* a prior transfer candidate is pending */
	PENTAX_RECONCILE_UNSAFE,		/* unsafe activity state; refuse capture */
	PENTAX_RECONCILE_UNREADABLE	/* conditions payload unreadable/short */
} PentaxReconcileDecision;

/* Research builds only: the vendor Pentax bodies whose capture flow we
 * exercise. See DEVELOPMENT_PLAN.md R0 and issue #19 (K-3 III Monochrome). */
int pentax_pid_is_research_capable (unsigned int pid);

/* Transfer-timeout decision for the in-flight transfer callback (issue #38):
 * a stalled camera (no bytes for a while) is a different failure from one
 * legitimately streaming a huge image; only the former trips the short bound. */
enum PentaxTransferTimeoutReason {
	PENTAX_TRANSFER_TIMEOUT_OK = 0,	/* within both bounds */
	PENTAX_TRANSFER_TIMEOUT_STALLED,	/* no progress for too long */
	PENTAX_TRANSFER_TIMEOUT_CEILING	/* absolute ceiling exceeded */
};
int pentax_transfer_timeout_reason (unsigned long long total_ms,
	unsigned long long idle_ms);

/* Recovery re-probe predicate: conditions readable and camera idle with no
 * pending candidate; the caller has already checked the PTP result code. */
int pentax_recovery_probe_ok (const unsigned char *data, size_t size);

/* Stale-candidate baseline for the pre-capture probe (issue #34): returns
 * the pending transfer handle when one is flagged, else 0. */
uint32_t pentax_stale_candidate_baseline (const unsigned char *data,
	size_t size);

/* Session reconciliation decision tree (issue #33). On STALE_CANDIDATE the
 * pending handle is recorded in *candidate_out; on UNREADABLE it is left
 * untouched. */
PentaxReconcileDecision pentax_reconcile_conditions (const unsigned char *data,
	size_t size, uint32_t *candidate_out);

/* Capture wait budget from camera conditions: long bulb timers, astrotracer
 * limits, or multi-shot composites extend the base timeout so slow captures
 * are not aborted while still in progress. */
unsigned int pentax_capture_timeout_ms (const PentaxConditions *conditions);

uint32_t pentax_get_u32le (const unsigned char *data);
int pentax_parse_live_view_geometry (const unsigned char *data, size_t size,
	PentaxLiveViewGeometry *geometry);
int pentax_parse_live_view_af_position (const unsigned char *data, size_t size,
	const PentaxLiveViewGeometry *geometry, uint16_t *x, uint16_t *y);
int pentax_encode_live_view_af_position (uint16_t x, uint16_t y,
	unsigned char data[8]);
int pentax_encode_live_view_zoom (uint16_t x, uint16_t y,
	uint8_t magnification, unsigned char data[12]);
int pentax_live_view_stop_response_ok (uint16_t response);
int pentax_live_view_zoom_fallback (uint8_t requested, uint16_t response,
	uint8_t *fallback);
int pentax_live_view_frame_should_retry (uint16_t response,
	unsigned int attempts, unsigned int elapsed_ms);
int pentax_parse_conditions (const unsigned char *data, size_t size,
	PentaxConditions *conditions);
int pentax_minimum_focus_displacement (uint32_t open_av_num, int direction,
	int32_t *displacement);
int pentax_lookup_model (uint16_t usb_vendor, uint16_t usb_product,
	const char *device_model, uint32_t *model_no, uint32_t *extension_version);
int pentax_model_uses_new_focus (uint32_t model_no);
int pentax_model_supports_exp_bracket (uint32_t model_no);
int pentax_model_supports_composition_adjust (uint32_t model_no);
int pentax_model_supports_movie_setting (uint32_t model_no);
int pentax_model_supports_pc_live_view (uint32_t model_no);
int pentax_model_supports_cross_process (uint32_t model_no);
int pentax_model_supports_card_writing_mode (uint32_t model_no);
int pentax_capture_buffer_write (PentaxCaptureBuffer *buffer,
	const unsigned char *data, size_t size);
int pentax_capture_buffer_seek (PentaxCaptureBuffer *buffer,
	unsigned int operation, int32_t displacement);
int pentax_candidate_filename (const unsigned char *data, uint32_t size,
		char *filename, size_t filename_size);
int pentax_jpeg_bounds (const unsigned char *data, size_t size,
		size_t *offset, size_t *length);
int pentax_transfer_run (PentaxCaptureBuffer *buffer,
	const PentaxTransferOps *operations);

#endif
