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
/* IT2 ModeEnableInfo.AstroTracer3 (MtpDevice.cs:5926): a CAPABILITY bit in the
 * offset-504 field (capability_flags), NOT an offset-320 astro status bit.  It
 * says "this body can do Astro Tracer", not "it is currently exposing". */
#define PENTAX_CONDITION_ASTROTRACER3              0x00000200U

/* IT2 ExpMode.AstroTracer (ExpMode.cs): the exposure-mode-dial value that
 * selects Astro Tracer.  Read from conditions offset 184 (exposure_mode).  This
 * is the signal IT2 uses to switch to the long-exposure / bulb-timer path and is
 * model-agnostic, so it works for the K-1 II (old-focus) as well as the K-3 III. */
#define PENTAX_EXP_MODE_ASTROTRACER 20U

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

/* Minimum number of candidates expected after the primary candidate has
 * been finalized, derived from GetAllConditions writing format at +524.
 * RAW+JPEG publishes two output objects, so one companion remains. */
unsigned int pentax_expected_extra_candidates (const unsigned char *data,
	size_t size);

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

/* Phase-specific exposure-duration budget (issue #111): the time the camera
 * needs to complete the exposure itself, before any post-exposure processing
 * or transfer-candidate publication.  For a Bulb timer this is the timer
 * value plus a small settle margin; for multi-shot composites it is the
 * per-shot budget times the shot count.  The result is clamped to the
 * absolute ceiling so a corrupt camera-reported value cannot wrap.  This is
 * deliberately separate from pentax_capture_timeout_ms() which also covers
 * post-exposure processing: a 120 s Bulb must not consume its entire 151 s
 * all-in budget before RAW processing even starts. */
unsigned int pentax_exposure_phase_ms (const PentaxConditions *conditions);

/* Post-capture readiness tri-state (issue #122 / libgphoto2 #73): only a VALID
 * conditions frame may prove the camera idle.  A failed or short read is
 * UNKNOWN, never IDLE — treating it as idle let Benro fire the next shutter
 * while the camera was still busy, wedging it until a mode toggle + USB reset. */
typedef enum {
	PENTAX_READINESS_IDLE = 0,	/* valid frame: no unsafe activity, no candidate */
	PENTAX_READINESS_BUSY = 1,	/* valid frame: unsafe activity or pending candidate */
	PENTAX_READINESS_UNKNOWN = 2 /* read failed or frame too short to trust */
} PentaxReadiness;

/* Classify a raw conditions frame.  data/size are the GetAllConditions payload
 * (may be NULL/0).  Returns IDLE only when the frame is valid AND proves idle;
 * BUSY when the frame is valid and shows activity or a pending candidate;
 * UNKNOWN when the read failed or the frame is shorter than MIN_SIZE. */
PentaxReadiness pentax_camera_readiness (const unsigned char *data,
		   unsigned int size);

/* Whether the post-capture idle wait applies to this capture (issue #122):
 * multi-shot composites, astro shift/tracer and bulb exposures can still be
 * processing after all candidates are consumed.  Ordinary single-shot JPEG/RAW
 * captures return immediately — Benro regains control through the normal
 * completion path without an invented readiness state machine. */
int pentax_capture_needs_idle_wait (const PentaxConditions *conditions);

/* Issue #122 (TA follow-up): fail-closed post-capture readiness wait.  Polls
 * read_cb (a conditions reader returning 0/PTP_RC_OK on success) until
 * pentax_camera_readiness() reports a POSITIVE IDLE transition, bounded by
 * max_ms.  Returns:
 *   1  when IDLE was actually observed;
 *   0  when the bound is exhausted while the camera is still BUSY or UNKNOWN -
 *      in which case the caller must treat the capture as not-ready
 *      (GP_ERROR_CAMERA_BUSY), never as a successful completion.  A failed read
 *      is UNKNOWN, never IDLE.
 *  -1  when cancel_cb (if non-NULL) reports a cancel while polling, so the wait
 *      exits promptly without consuming the remaining bound and the caller
 *      propagates GP_ERROR_CANCEL.  This preserves capture cancellation across
 *      the refactor to this helper: the pre-refactor inline loop checked
 *      gp_context_cancel() on every iteration (issue #122 follow-up).
 * cancel_cb is invoked once per poll iteration with cancel_user_data and returns
 * non-zero when the operation has been cancelled.  Pass NULL for both cancel
 * parameters to disable cancellation checking. */
int pentax_wait_for_idle (int (*read_cb) (void *user_data, unsigned char **data,
                      unsigned int *size),
              void *user_data,
              int (*cancel_cb) (void *cancel_user_data),
              void *cancel_user_data,
              int max_ms);

/* Whether the camera is currently in Astro Tracer mode, per IT2's own signal:
 * the exposure-mode dial value (offset 184) equals ExpMode.AstroTracer (20).
 * This is the authoritative "in astro mode" test — the offset-504 capability
 * bit (PENTAX_CONDITION_ASTROTRACER3) only says the body CAN do it, and the
 * offset-320 status bits describe shift/aperture sub-states, not the mode.
 * Model-agnostic, so it covers the K-1 II as well as the K-3 III. */
int pentax_conditions_in_astro_mode (const PentaxConditions *conditions);

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
/* Issue #86: should the preview frame fetch be retried?  0xa008 (NoUpdateImage)
 * is always retried within the bounded window.  0x2002 (GeneralError) is retried
 * ONLY when the data phase was zero bytes (the K-1 II empty-frame transition);
 * a non-empty 0x2002 carries a genuine failure context and remains terminal. */
int pentax_live_view_frame_should_retry (uint16_t response,
	unsigned int attempts, unsigned int elapsed_ms,
	unsigned int data_size);
int pentax_parse_conditions (const unsigned char *data, size_t size,
	PentaxConditions *conditions);
int pentax_minimum_focus_displacement (uint32_t open_av_num, int direction,
	int32_t *displacement);
int pentax_old_focus_protocol_direction (int direction,
	uint32_t *protocol_direction);
int pentax_lookup_model (uint16_t usb_vendor, uint16_t usb_product,
	const char *device_model, uint32_t *model_no, uint32_t *extension_version);
int pentax_model_uses_new_focus (uint32_t model_no);
int pentax_model_supports_exp_bracket (uint32_t model_no);
int pentax_model_supports_composition_adjust (uint32_t model_no);
int pentax_model_supports_movie_setting (uint32_t model_no);
int pentax_model_supports_pc_live_view (uint32_t model_no);
/* Format a shutter-speed UINT64 wire value (high 32 = denominator, low 32 =
 * numerator) for display: "Auto", "<n>s" for whole-second timer values
 * (Bulb timer), "1/<n>" and "<a>/<b>" for fractions.  Returns 0 on success. */
int pentax_format_shutter_speed (uint64_t value, char *buf, size_t buflen);
int pentax_model_supports_cross_process (uint32_t model_no);
int pentax_model_supports_card_writing_mode (uint32_t model_no);
int pentax_model_supports_writing_file_format (uint32_t model_no);
int pentax_wff_quality_byte (const char *value);
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

/* Bounded reconciliation of extra transfer candidates from a dual-format
 * exposure (issue #73).  After the primary candidate has been transferred
 * and finalized, this loop detects and consumes any remaining candidates
 * belonging to the same already-initiated exposure so the camera is left
 * ready for the next shutter.
 *
 * The loop is bounded by max_count (number of extra candidates to consume)
 * and max_ms (total wall-clock budget in milliseconds). min_count is the
 * minimum companion obligation reported by the camera output configuration;
 * it is not derived from physical exposure count. Each iteration:
 *   1. Reads GetAllConditions via get_conditions. An empty response completes
 *      only after min_count candidates have been finalized.
 *   2. Transfers the pending candidate into a fresh buffer via
 *      transfer_candidate.
 *   3. Finalizes it via delete_candidate.
 *   4. Records the candidate filename (from get_candidate_info +
 *      pentax_candidate_filename) in names[reconciled_count].
 *
 * On success *reconciled_count is set to the number of extras consumed
 * (0 when none were pending).  On failure the count reflects how many
 * were completed before the error.  The pre-capture stale-candidate
 * barrier (issue #34) is NOT weakened: this function only runs AFTER a
 * successful primary transfer+finalize within the same exposure.
 */
typedef struct {
	void *user_data;
	/* Read GetAllConditions; caller frees *data on GP_OK. */
	int (*get_conditions) (void *user_data, unsigned char **data,
		size_t *size);
	/* Get transfer candidate info (type 0); caller frees *data. */
	int (*get_candidate_info) (void *user_data, unsigned char **data,
		size_t *size);
	/* Transfer the current pending candidate into buffer. */
	int (*transfer_candidate) (void *user_data, PentaxCaptureBuffer *buffer);
	/* Finalize (delete) the current candidate on the camera. */
	int (*delete_candidate) (void *user_data);
	/* Cancellation check; return non-zero to stop. */
	int (*is_cancelled) (void *user_data);
} PentaxReconcileOps;

int pentax_reconcile_extra_candidates (const PentaxReconcileOps *ops,
	int max_count, unsigned int max_ms, unsigned int min_count,
	char (*names)[128], int *reconciled_count);

#endif
