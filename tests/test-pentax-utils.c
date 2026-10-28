#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gphoto2/gphoto2-result.h>

#include "ptp.h"
#include "pentax-utils.h"

#define CHECK(expression) do { \
	if (!(expression)) { \
		fprintf (stderr, "FAIL line %d: %s\n", __LINE__, #expression); \
		free (buffer.data); \
		return 1; \
	} \
} while (0)

typedef struct {
	uint8_t commands[8];
	int32_t command_info[8];
	size_t command_count;
	size_t command_index;
	uint32_t requests[8];
	size_t request_count;
	int command_error_at;
	int block_error_at;
	int zero_block;
	int oversized_block;
	/* When >= 0, the block at that index returns only part of the
	 * requested bytes (issue #35 short-block regression). */
	int short_block_at;
	uint32_t short_block_bytes;
	int cancelled;
	int timed_out;
} MockTransfer;

static int
mock_get_command (void *user_data, uint8_t *operation, int32_t *operation_info)
{
	MockTransfer *mock = user_data;

	if ((mock->command_error_at >= 0) &&
	    ((int)mock->command_index == mock->command_error_at))
		return GP_ERROR_IO;
	if (mock->command_index >= mock->command_count)
		return GP_ERROR_CORRUPTED_DATA;
	*operation = mock->commands[mock->command_index];
	*operation_info = mock->command_info[mock->command_index];
	mock->command_index++;
	return GP_OK;
}

static int
mock_get_block (void *user_data, uint32_t requested, unsigned char **data,
		uint32_t *transferred)
{
	MockTransfer *mock = user_data;
	size_t index = mock->request_count++;

	if (index < sizeof (mock->requests) / sizeof (mock->requests[0]))
		mock->requests[index] = requested;
	if ((mock->block_error_at >= 0) && ((int)index == mock->block_error_at)) {
		*data = malloc (1);
		*transferred = 1;
		return GP_ERROR_IO;
	}
	if (mock->zero_block) {
		*data = NULL;
		*transferred = 0;
		return GP_OK;
	}
	if ((mock->short_block_at >= 0) && ((int)index == mock->short_block_at)) {
		uint32_t partial = mock->short_block_bytes;

		/* A configured size at or above the request is not a short
		 * block; serve the full request. */
		if (partial && (partial < requested)) {
			*transferred = partial;
			*data = malloc (*transferred);
			if (!*data)
				return GP_ERROR_NO_MEMORY;
			memset (*data, (int)('A' + index), *transferred);
			return GP_OK;
		}
	}
	*transferred = mock->oversized_block ? requested + 1 : requested;
	*data = malloc (*transferred);
	if (!*data)
		return GP_ERROR_NO_MEMORY;
	memset (*data, (int)('A' + index), *transferred);
	return GP_OK;
}

static int
mock_cancelled (void *user_data)
{
	return ((MockTransfer *)user_data)->cancelled;
}

static int
mock_timed_out (void *user_data)
{
	return ((MockTransfer *)user_data)->timed_out;
}

static void
mock_init (MockTransfer *mock)
{
	memset (mock, 0, sizeof (*mock));
	mock->command_error_at = -1;
	mock->block_error_at = -1;
	mock->short_block_at = -1;
}

static void
put_u32le (unsigned char *data, size_t offset, uint32_t value)
{
	data[offset] = (unsigned char)value;
	data[offset + 1] = (unsigned char)(value >> 8);
	data[offset + 2] = (unsigned char)(value >> 16);
	data[offset + 3] = (unsigned char)(value >> 24);
}

int
main (void)
{
	int32_t displacement = 0;
	PentaxCaptureBuffer buffer = {0};
	PentaxConditions conditions, unchanged;

	CHECK (pentax_bulb_action_mode_supported (PENTAX_EXP_MODE_BULB));
	CHECK (pentax_bulb_action_mode_supported (PENTAX_EXP_MODE_BULB_LENS));
	CHECK (pentax_bulb_action_mode_supported (PENTAX_EXP_MODE_ASTROTRACER));
	CHECK (!pentax_bulb_action_mode_supported (8));
	CHECK (!pentax_bulb_action_mode_supported (4));
	CHECK (pentax_bulb_action_model_supported (PENTAX_MODEL_K1_MARK_II, 0x0183));
	CHECK (pentax_bulb_action_model_supported (PENTAX_MODEL_K3_MARK_III, 0x0189));
	CHECK (!pentax_bulb_action_model_supported (PENTAX_MODEL_K3_MARK_III, 0x018f));
	CHECK (!pentax_bulb_action_model_supported (0, 0));

	/* Camera-timed Bulb fallback (#239): pgphoto drives "Bulb with seconds
	 * selected" by sleeping between its start and stop edges, so the requested
	 * length is only recoverable from the measured hold.  Rounding must land on
	 * the second the user picked despite scheduling jitter, a sub-second hold can
	 * never yield a zero shutter speed, and a clock that went backwards (or a
	 * zeroed start timestamp) must not produce a huge or wrapped duration. */
	CHECK (pentax_bulb_hold_to_seconds (1000, 3000) == 2);
	CHECK (pentax_bulb_hold_to_seconds (1000, 3400) == 2);
	CHECK (pentax_bulb_hold_to_seconds (1000, 3500) == 3);
	CHECK (pentax_bulb_hold_to_seconds (1000, 6000) == 5);
	CHECK (pentax_bulb_hold_to_seconds (1000, 12000) == 11);
	CHECK (pentax_bulb_hold_to_seconds (1000, 1050) == 1);
	CHECK (pentax_bulb_hold_to_seconds (1000, 1000) == 1);
	CHECK (pentax_bulb_hold_to_seconds (5000, 1000) == 1);
	CHECK (pentax_bulb_hold_to_seconds (0, 0) == 1);
	CHECK (pentax_bulb_hold_to_seconds (UINT64_MAX, 10) == 1);

	/* Whole-second timer layout, matching _put_Ricoh_ShutterSpeed: numerator in
	 * the low 32 bits, denominator in the high 32 bits. */
	CHECK (pentax_whole_second_shutter_value (2) == (((uint64_t) 1 << 32) | 2));
	CHECK (pentax_whole_second_shutter_value (1) == (((uint64_t) 1 << 32) | 1));
	CHECK ((pentax_whole_second_shutter_value (300) & 0xffffffffu) == 300);
	CHECK ((pentax_whole_second_shutter_value (300) >> 32) == 1);

	/* Issue #241: the Bulb/seconds entries the K-3 III publishes in a populated
	 * shutter list.  The body omits them entirely in B mode with no JPEG, which
	 * leaves the list empty and the app with nothing to select.  These are the
	 * body's own values, so the synthesised list matches a populated one. */
	CHECK (PENTAX_BULB_LIST_SECONDS_COUNT == 13);
	CHECK (pentax_bulb_list_seconds[0] == 1);
	CHECK (pentax_bulb_list_seconds[PENTAX_BULB_LIST_SECONDS_COUNT - 1] == 30);
	{
		unsigned int bi;
		for (bi = 0; bi < PENTAX_BULB_LIST_SECONDS_COUNT; bi++) {
			uint64_t v = pentax_whole_second_shutter_value (
					pentax_bulb_list_seconds[bi]);
			CHECK (pentax_shutter_value_is_bulb_seconds (v));
			/* The wire value must round-trip through the formatter the widget
			 * uses, so the app sees the same label the camera would have sent. */
			char lbl[32];
			CHECK (pentax_format_shutter_speed (v, lbl, sizeof (lbl)) == 0);
			CHECK (lbl[0] == '0' && lbl[1] == '0' && lbl[2] == ':');
		}
	}
	/* Issue #241 review: the synthesis must be constrained to the body whose
	 * published list the values came from.  A K-1 II, K-01 or the Monochrome
	 * K-3 III has its own list and must never be handed these choices. */
	CHECK (pentax_shutter_list_may_synthesise_bulb (PENTAX_MODEL_K3_MARK_III,
		0x0189));
	CHECK (!pentax_shutter_list_may_synthesise_bulb (PENTAX_MODEL_K3_MARK_III,
		0x018f));
	CHECK (!pentax_shutter_list_may_synthesise_bulb (PENTAX_MODEL_K1_MARK_II,
		0x0183));
	CHECK (!pentax_shutter_list_may_synthesise_bulb (PENTAX_MODEL_KP, 0x017f));
	CHECK (!pentax_shutter_list_may_synthesise_bulb (PENTAX_MODEL_K70, 0x017d));
	CHECK (!pentax_shutter_list_may_synthesise_bulb (0, 0));

	/* A plain "1/N" shutter speed must not be mistaken for a Bulb entry: it has
	 * numerator 1 but a large denominator, not denominator 1. */
	CHECK (!pentax_shutter_value_is_bulb_seconds (((uint64_t) 4000 << 32) | 1));
	CHECK (!pentax_shutter_value_is_bulb_seconds (0));
	CHECK (!pentax_shutter_value_is_bulb_seconds (
			pentax_whole_second_shutter_value (7)));   /* 7 s is not in the list */
	PentaxLiveViewGeometry geometry, unchanged_geometry;
	unsigned char condition_data[532] = {0};
	unsigned char geometry_data[20] = {0};
	unsigned char af_data[16] = {0}, encoded_af[8], encoded_zoom[12];
	const unsigned char filename[] = {
		1, 2, 3, 11,
		'I', 0, 'M', 0, 'G', 0, '0', 0, '0', 0, '0', 0, '1', 0, '.', 0,
		'J', 0, 'P', 0, 'G', 0
	};
	unsigned char malformed[sizeof (filename)];
	const unsigned char first[] = {1, 2, 3, 4};
	const unsigned char patch[] = {9, 8};
	char name[32];
	size_t jpeg_offset = 99, jpeg_length = 99;
	uint32_t model_no = 99, extension_version = 99;
	uint16_t af_x = 99, af_y = 99;
	uint8_t fallback = 99;
	MockTransfer mock;
	PentaxTransferOps transfer_operations;
	unsigned int i;

	CHECK (pentax_capture_cleanup_decision (0) ==
		PENTAX_CAPTURE_CLEANUP_ABORT);
	CHECK (pentax_capture_cleanup_decision (1) ==
		PENTAX_CAPTURE_CLEANUP_PRESERVE_CANDIDATE);

	CHECK (pentax_get_u32le ((const unsigned char *)"\x78\x56\x34\x12") ==
		0x12345678U);

	/* Keep the wire constants aligned with IMAGE Transmitter 2's MtpOpCode,
	 * ShootingFormat, WritingFormatSetting, and TransFileFormat declarations. */
	CHECK (PTP_OC_PENTAX_GetTransferCandidateFileInfo == 0x900b);
	CHECK (PTP_OC_PENTAX_GetCamFileOperationCommand == 0x900c);
	CHECK (PTP_OC_PENTAX_GetTransferFileDataBlock == 0x900d);
	CHECK (PTP_OC_PENTAX_DeleteTransferCandidate == 0x900e);
	CHECK (PTP_OC_PENTAX_GetAllConditions == 0x900f);
	CHECK (PTP_OC_PENTAX_InitiateCapture == 0x9011);
	CHECK (PTP_OC_PENTAX_TerminateCapture == 0x9012);
	CHECK (PTP_OC_PENTAX_FocusControl == 0x9016);
	CHECK (PTP_OC_PENTAX_FocusControlNew == 0x9017);
	CHECK (PTP_DPC_PENTAX_WritingFileFormat == 0xd01b);
	CHECK (PTP_PENTAX_SHOOTING_FORMAT_JPEG == 0);
	CHECK (PTP_PENTAX_SHOOTING_FORMAT_PEF == 1);
	CHECK (PTP_PENTAX_SHOOTING_FORMAT_DNG == 2);
	CHECK (PTP_PENTAX_SHOOTING_FORMAT_PEF_JPEG == 3);
	CHECK (PTP_PENTAX_SHOOTING_FORMAT_DNG_JPEG == 4);
	CHECK (PTP_PENTAX_SHOOTING_FORMAT_TIFF == 5);
	CHECK (PTP_PENTAX_WRITING_FORMAT_JPEG == 0);
	CHECK (PTP_PENTAX_WRITING_FORMAT_RAW == 1);
	CHECK (PTP_PENTAX_WRITING_FORMAT_RAW_JPEG == 2);
	CHECK (PTP_PENTAX_WRITING_FORMAT_TIFF == 3);
	CHECK (PTP_PENTAX_TRANSFER_FILE_CAPTURE == 0);
	CHECK (PTP_PENTAX_TRANSFER_FILE_JPEG == 1);
	CHECK (PTP_PENTAX_TRANSFER_FILE_PEF == 2);
	CHECK (PTP_PENTAX_TRANSFER_FILE_DNG == 3);
	CHECK (PTP_PENTAX_TRANSFER_FILE_TIFF == 4);
	CHECK (PTP_PENTAX_TRANSFER_FILE_FULL == 253);
	CHECK (PTP_PENTAX_TRANSFER_FILE_SUB == 254);
	CHECK (PTP_PENTAX_TRANSFER_FILE_THUMB == 255);
	/* The three IT2 enum domains intentionally overlap at zero but have
	 * different meanings.  These guards stop a future refactor from using a
	 * ShootingFormat or transfer-file value as a WritingFormatSetting. */
	CHECK ((int)PTP_PENTAX_SHOOTING_FORMAT_PEF ==
		(int)PTP_PENTAX_WRITING_FORMAT_RAW);
	CHECK ((int)PTP_PENTAX_SHOOTING_FORMAT_DNG ==
		(int)PTP_PENTAX_WRITING_FORMAT_RAW_JPEG);
	CHECK ((int)PTP_PENTAX_TRANSFER_FILE_CAPTURE ==
		(int)PTP_PENTAX_WRITING_FORMAT_JPEG);
	CHECK ((int)PTP_PENTAX_TRANSFER_FILE_PEF !=
		(int)PTP_PENTAX_SHOOTING_FORMAT_PEF);

	/* Regression: v9g imagequality 0xd01b byte-7 encoding (issue #71 / #55).
	 * The old code computed (uint8_t)(2 - stars), which produced 0xff for
	 * "fine" and was off-by-one for the other two. The encoder must map to
	 * the documented IT2 values: 0=fine, 1=normal, 2=basic, and fail closed
	 * (return -1) on any unknown label so a bogus byte is never written. */
	CHECK (pentax_wff_quality_byte ("fine") == 0);
	CHECK (pentax_wff_quality_byte ("normal") == 1);
	CHECK (pentax_wff_quality_byte ("basic") == 2);
	CHECK (pentax_wff_quality_byte ("") == -1);
	CHECK (pentax_wff_quality_byte ("FINE") == -1);   /* case-sensitive */
	CHECK (pentax_wff_quality_byte (NULL) == -1);
	CHECK (pentax_wff_format_byte ("JPEG") == 0);
	CHECK (pentax_wff_format_byte ("RAW") == 1);
	CHECK (pentax_wff_format_byte ("RAW+JPEG") == 2);
	CHECK (pentax_wff_format_byte ("TIFF") == 3);
	/* PEF/DNG are RawKind values, not imageformat choices. */
	CHECK (pentax_wff_format_byte ("PEF") == -1);
	CHECK (pentax_wff_format_byte ("DNG") == -1);
	CHECK (pentax_wff_format_byte ("PEF+JPEG") == -1);
	CHECK (pentax_wff_format_byte ("DNG+JPEG") == -1);
	CHECK (pentax_wff_format_byte ("jpeg") == -1);
	CHECK (pentax_wff_format_byte (NULL) == -1);
	CHECK (pentax_wff_format_supported (PENTAX_MODEL_K1_MARK_II, "JPEG"));
	CHECK (pentax_wff_format_supported (PENTAX_MODEL_K1, "RAW"));
	CHECK (pentax_wff_format_supported (PENTAX_MODEL_K3_MARK_III,
		"RAW+JPEG"));
	CHECK (pentax_wff_format_supported (
		PENTAX_MODEL_K3_MARK_III_MONOCHROME, "JPEG"));
	CHECK (!pentax_wff_format_supported (PENTAX_MODEL_K1_MARK_II, "TIFF"));
	CHECK (!pentax_wff_format_supported (PENTAX_MODEL_K3_MARK_III, "PEF"));
	CHECK (!pentax_wff_format_supported (0, "JPEG"));
	CHECK (pentax_wff_raw_kind_byte ("PEF") == PTP_PENTAX_RAW_KIND_PEF);
	CHECK (pentax_wff_raw_kind_byte ("DNG") == PTP_PENTAX_RAW_KIND_DNG);
	CHECK (!strcmp (pentax_wff_raw_kind_label (PTP_PENTAX_RAW_KIND_PEF), "PEF"));
	CHECK (!strcmp (pentax_wff_raw_kind_label (PTP_PENTAX_RAW_KIND_DNG), "DNG"));
	CHECK (pentax_wff_raw_kind_label (2) == NULL);
	CHECK (pentax_wff_raw_kind_byte ("RAW") == -1);
	{
		unsigned char wff10[10] = { 6, 0, 0, 0, 2, 2, 4, 1, 0, 1 };
		unsigned char wff11[11] = { 7, 0, 0, 0, 2, 2, 4, 1, 0, 1, 2 };
		unsigned char malformed_wff[11];
		unsigned char original_wff10[10];
		memcpy (malformed_wff, wff11, sizeof (malformed_wff));
		memcpy (original_wff10, wff10, sizeof (original_wff10));
		CHECK (pentax_wff_payload_is_valid (wff10, sizeof (wff10)));
		CHECK (pentax_wff_payload_is_valid (wff11, sizeof (wff11)));
		CHECK (pentax_wff_set_format (wff10, sizeof (wff10), "RAW") == 0);
		CHECK (wff10[4] == 1 && wff10[5] == 1);
		CHECK (wff10[8] == original_wff10[8] && wff10[9] == original_wff10[9]);
		CHECK (pentax_wff_set_raw_kind (wff10, sizeof (wff10), "DNG") == 0);
		CHECK (wff10[8] == PTP_PENTAX_RAW_KIND_DNG);
		CHECK (wff10[4] == 1 && wff10[5] == 1 && wff10[9] == original_wff10[9]);
		CHECK (pentax_wff_set_raw_kind (wff10, sizeof (wff10), "PEF") == 0);
		CHECK (wff10[8] == PTP_PENTAX_RAW_KIND_PEF);
		CHECK (pentax_wff_set_format (wff10, sizeof (wff10), "PEF") == -1);
		CHECK (pentax_wff_set_raw_kind (wff10, sizeof (wff10), "RAW") == -1);
		wff10[4] = 3;
		wff10[5] = 3;
		CHECK (pentax_wff_payload_is_valid (wff10, sizeof (wff10)));
		wff10[4] = 4;
		wff10[5] = 4;
		CHECK (!pentax_wff_payload_is_valid (wff10, sizeof (wff10)));
		malformed_wff[0] = 6;
		CHECK (!pentax_wff_payload_is_valid (malformed_wff,
			sizeof (malformed_wff)));
		memcpy (malformed_wff, wff11, sizeof (malformed_wff));
		malformed_wff[4] = 6;
		CHECK (!pentax_wff_payload_is_valid (malformed_wff,
			sizeof (malformed_wff)));
		memcpy (malformed_wff, wff11, sizeof (malformed_wff));
		malformed_wff[7] = 3;
		CHECK (!pentax_wff_payload_is_valid (malformed_wff,
			sizeof (malformed_wff)));
		CHECK (!pentax_wff_payload_is_valid (wff10, 9));
	}

	put_u32le (geometry_data, 4, 480U << 16 | 720U);
	put_u32le (geometry_data, 8, 400U << 16 | 640U);
	put_u32le (geometry_data, 12, 360U << 16 | 600U);
	put_u32le (geometry_data, 16, 40U << 16 | 60U);
	memset (&unchanged_geometry, 0xa5, sizeof (unchanged_geometry));
	geometry = unchanged_geometry;
	CHECK (pentax_parse_live_view_geometry (geometry_data, 19, &geometry) ==
		GP_ERROR_CORRUPTED_DATA);
	CHECK (!memcmp (&geometry, &unchanged_geometry, sizeof (geometry)));
	CHECK (pentax_parse_live_view_geometry (geometry_data,
		sizeof (geometry_data), &geometry) == GP_OK);
	CHECK ((geometry.area_width == 720) && (geometry.area_height == 480) &&
		(geometry.active_width == 640) && (geometry.active_height == 400) &&
		(geometry.contrast_af_active_width == 600) &&
		(geometry.contrast_af_active_height == 360) &&
		(geometry.contrast_af_spot_width == 60) &&
		(geometry.contrast_af_spot_height == 40));
	/* Only the 8-byte coordinate form parses; every 0-7 byte response is
	 * rejected and outputs stay untouched on failure. */
	CHECK (pentax_parse_live_view_af_position (af_data, 4, &geometry,
		&af_x, &af_y) == GP_ERROR_CORRUPTED_DATA);
	CHECK ((af_x == 99) && (af_y == 99));
	CHECK (pentax_parse_live_view_af_position (af_data, 3, &geometry,
		&af_x, &af_y) == GP_ERROR_CORRUPTED_DATA);
	CHECK ((af_x == 99) && (af_y == 99));
	CHECK (pentax_parse_live_view_af_position (af_data, 0, &geometry,
		&af_x, &af_y) == GP_ERROR_CORRUPTED_DATA);
	CHECK ((af_x == 99) && (af_y == 99));
	af_data[0] = 2; /* encoder tag required by the parser (issue #26) */
	af_data[4] = 100; af_data[6] = 200;
	CHECK (pentax_parse_live_view_af_position (af_data, 8, &geometry,
		&af_x, &af_y) == GP_OK);
	CHECK ((af_x == 100) && (af_y == 200));
	/* Longer payloads are rejected: only the exact 8-byte form parses
	 * (issue #26). */
	CHECK (pentax_parse_live_view_af_position (af_data, 12, &geometry,
		&af_x, &af_y) == GP_ERROR_CORRUPTED_DATA);
	CHECK ((af_x == 100) && (af_y == 200));
	CHECK (pentax_parse_live_view_af_position (af_data, 16, &geometry,
		&af_x, &af_y) == GP_ERROR_CORRUPTED_DATA);
	CHECK ((af_x == 100) && (af_y == 200));
	CHECK (pentax_parse_live_view_af_position (af_data, 7, &geometry,
		&af_x, &af_y) == GP_ERROR_CORRUPTED_DATA);
	CHECK ((af_x == 100) && (af_y == 200));
	/* Out-of-range coordinates are rejected without touching outputs. */
	af_x = 99; af_y = 99;
	af_data[4] = 0xd1; af_data[5] = 0x02; /* x = 720 >= area_width */
	CHECK (pentax_parse_live_view_af_position (af_data, 8, &geometry,
		&af_x, &af_y) == GP_ERROR_CORRUPTED_DATA);
	CHECK ((af_x == 99) && (af_y == 99));
	af_data[4] = 100; af_data[5] = 0;
	af_data[6] = 0xe1; af_data[7] = 0x01; /* y = 480 >= area_height */
	CHECK (pentax_parse_live_view_af_position (af_data, 8, &geometry,
		&af_x, &af_y) == GP_ERROR_CORRUPTED_DATA);
	CHECK ((af_x == 99) && (af_y == 99));
	af_data[6] = 200; af_data[7] = 0;
	/* Byte 0 must carry the encoder tag 2; other header bytes are opaque. */
	memset (af_data, 0xff, 4);
	af_data[0] = 2;
	CHECK (pentax_parse_live_view_af_position (af_data, 8, &geometry,
		&af_x, &af_y) == GP_OK);
	CHECK ((af_x == 100) && (af_y == 200));
	af_data[5] = 3;
	CHECK (pentax_parse_live_view_af_position (af_data, 8, &geometry,
		&af_x, &af_y) == GP_ERROR_CORRUPTED_DATA);
	af_data[5] = 0;
	af_data[0] = 3;
	CHECK (pentax_parse_live_view_af_position (af_data, 8, &geometry,
		&af_x, &af_y) == GP_ERROR_CORRUPTED_DATA);
	CHECK (pentax_encode_live_view_af_position (300, 200, encoded_af) == GP_OK);
	CHECK ((encoded_af[0] == 2) && (encoded_af[4] == 44) &&
		(encoded_af[5] == 1) && (encoded_af[6] == 200));
	CHECK (pentax_encode_live_view_zoom (300, 200, 16, encoded_zoom) == GP_OK);
	CHECK ((encoded_zoom[0] == 4) && (encoded_zoom[4] == 44) &&
		(encoded_zoom[5] == 1) && (encoded_zoom[6] == 200) &&
		(encoded_zoom[8] == 16));
	CHECK (pentax_encode_live_view_zoom (0, 0, 0, encoded_zoom) ==
		GP_ERROR_BAD_PARAMETERS);
	CHECK (pentax_live_view_stop_response_ok (0x2001));
	CHECK (pentax_live_view_stop_response_ok (0xa005));
	CHECK (!pentax_live_view_stop_response_ok (0x2019));
	CHECK (pentax_live_view_zoom_fallback (16, 0x201c, &fallback) == 1);
	CHECK (fallback == 10);
	fallback = 99;
	CHECK (pentax_live_view_zoom_fallback (10, 0x201c, &fallback) == 0);
	CHECK (fallback == 99);
	CHECK (pentax_live_view_frame_should_retry (0xa008, 1, 0, 0));
	CHECK (pentax_live_view_frame_should_retry (0xa008, 29, 1499, 0));
	CHECK (!pentax_live_view_frame_should_retry (0xa008, 30, 0, 0));
	CHECK (!pentax_live_view_frame_should_retry (0xa008, 1, 1500, 0));
	/* Issue #86: the K-1 II empty-frame transition returns 0x2002 with a
	 * zero-byte data phase; it must be retried within the same bounded window
	 * so a transient empty frame does not tear down live view and take the
	 * camera off USB. */
	/* Zero-byte 0x2002 (the K-1 II empty-frame transition) is retried. */
	CHECK (pentax_live_view_frame_should_retry (0x2002, 1, 0, 0));
	CHECK (pentax_live_view_frame_should_retry (0x2002, 29, 1499, 0));
	CHECK (!pentax_live_view_frame_should_retry (0x2002, 30, 0, 0));
	CHECK (!pentax_live_view_frame_should_retry (0x2002, 1, 1500, 0));
	/* Non-empty 0x2002 (genuine GeneralError with a data phase) is NOT retried. */
	CHECK (!pentax_live_view_frame_should_retry (0x2002, 1, 0, 64));
	CHECK (!pentax_live_view_frame_should_retry (0x2002, 1, 0, 1));
	CHECK (!pentax_live_view_frame_should_retry (0x2019, 1, 0, 0));
	CHECK (pentax_lookup_model (0x25fb, 0x0189, "PENTAX K-3 Mark III",
		&model_no, &extension_version));
	CHECK ((model_no == 78420) && (extension_version == 1));
	CHECK (pentax_lookup_model (0x25fb, 0x0183, "PENTAX K-1 Mark II",
		&model_no, &extension_version));
	CHECK ((model_no == 78400) && (extension_version == 1));
	CHECK (pentax_lookup_model (0x25fb, 0x0179, "PENTAX K-1",
		&model_no, &extension_version));
	CHECK ((model_no == PENTAX_MODEL_K1) && (extension_version == 1));
	/* The K-1 II declares a second product ID in its own firmware image and it
	 * has been seen on hardware; it must resolve identically (issue #179). */
	CHECK (pentax_lookup_model (0x25fb, 0x0182, "PENTAX K-1 Mark II",
		&model_no, &extension_version));
	CHECK ((model_no == 78400) && (extension_version == 1));
	/* The original K-1 (0x0179) is a different body and must not be reached
	 * through either K-1 II PID. */
	CHECK (!pentax_lookup_model (0x25fb, 0x0182, "PENTAX K-1",
		&model_no, &extension_version));
	CHECK (pentax_model_uses_new_focus (PENTAX_MODEL_K3_MARK_III));
	CHECK (pentax_model_uses_new_focus (PENTAX_MODEL_K3_MARK_III_MONOCHROME));
	CHECK (!pentax_model_uses_new_focus (PENTAX_MODEL_K1_MARK_II));
	CHECK (!pentax_model_uses_new_focus (PENTAX_MODEL_K1));
	CHECK (!pentax_model_uses_new_focus (0));
	{
		PentaxConditions conditions;
		uint32_t release_mode;

		memset (&conditions, 0, sizeof (conditions));
		conditions.exposure_mode = 8;
		conditions.drive_mode = 1;
		CHECK (pentax_it2_release_mode (1, &conditions,
			&release_mode) == GP_OK);
		CHECK (release_mode == 0);
		conditions.exposure_mode = 9;
		CHECK (pentax_it2_release_mode (1, &conditions,
			&release_mode) == GP_OK);
		CHECK (release_mode == 2);
		conditions.capability_flags = PENTAX_CONDITION_BULB_TIMER;
		CHECK (pentax_it2_release_mode (1, &conditions,
			&release_mode) == GP_OK);
		CHECK (release_mode == 0);
		conditions.capability_flags = 0;
		conditions.exposure_mode = 8;
		conditions.activity_flags = PENTAX_CONDITION_ACTIVITY_MOVIE_MODE;
		CHECK (pentax_it2_release_mode (1, &conditions,
			&release_mode) == GP_OK);
		CHECK (release_mode == 3);
		conditions.drive_mode = 4;
		CHECK (pentax_it2_release_mode (1, &conditions,
			&release_mode) == GP_OK);
		CHECK (release_mode == 1);
		CHECK (pentax_it2_release_mode (0, &conditions,
			&release_mode) == GP_OK);
		CHECK (release_mode == 0);
		CHECK (pentax_it2_release_mode (1, NULL, &release_mode) ==
			GP_ERROR_BAD_PARAMETERS);
		CHECK (pentax_it2_release_mode (1, &conditions, NULL) ==
			GP_ERROR_BAD_PARAMETERS);
	}
	/* IMAGE Transmitter 2 starts PC live view directly on both K-1 bodies;
	 * the K-3 III remains on the observed-state path. */
	CHECK (pentax_model_uses_it2_direct_live_view_start (PENTAX_MODEL_K1));
	CHECK (pentax_model_uses_it2_direct_live_view_start (PENTAX_MODEL_K1_MARK_II));
	CHECK (!pentax_model_uses_it2_direct_live_view_start (PENTAX_MODEL_K3_MARK_III));
	CHECK (!pentax_model_uses_it2_direct_live_view_start (PENTAX_MODEL_KP));
	CHECK (pentax_model_capabilities (PENTAX_MODEL_K3_MARK_III) != NULL);
	CHECK (pentax_model_capabilities (PENTAX_MODEL_K3_MARK_III)->k3iii_family);
	CHECK (pentax_model_capabilities (PENTAX_MODEL_K3_MARK_III)->supports_pc_live_view);
	CHECK (pentax_model_capabilities (PENTAX_MODEL_K1_MARK_II)->focus_family ==
		PENTAX_FOCUS_FAMILY_OLD);
	/* Keep the source-authorised model boundaries explicit.  These are policy
	 * fixtures, not claims that an unconnected body has been qualified.  In
	 * particular, K-1/K-1 II use the old-focus family and dual-slot/file-format
	 * controls, while the K-3 III family owns the newer live-view/CI controls.
	 * This prevents a future capability-table edit from silently widening K-3
	 * controls to the K-1 family or from giving Mono colour-only product gates. */
	{
		const PentaxModelCapabilities *k1ii =
			pentax_model_capabilities (PENTAX_MODEL_K1_MARK_II);
		const PentaxModelCapabilities *k1 =
			pentax_model_capabilities (PENTAX_MODEL_K1);
		const PentaxModelCapabilities *k3 =
			pentax_model_capabilities (PENTAX_MODEL_K3_MARK_III);
		const PentaxModelCapabilities *mono =
			pentax_model_capabilities (PENTAX_MODEL_K3_MARK_III_MONOCHROME);

		CHECK (k1ii && !k1ii->k3iii_family);
		CHECK (k1ii->focus_family == PENTAX_FOCUS_FAMILY_OLD);
		CHECK (k1ii->supports_dual_slot);
		CHECK (k1ii->supports_new_transfer);
		CHECK (k1ii->supports_movie);
		CHECK (k1ii->uses_it2_direct_live_view_start);
		CHECK (!k1ii->supports_movie_sub_image);
		CHECK (!k1ii->supports_tv_bracket);
		CHECK (!k1ii->supports_av_bracket);
		CHECK (!k1ii->supports_exp_bracket);
		CHECK (!k1ii->supports_composition_adjust);
		CHECK (!k1ii->supports_movie_setting);
		CHECK (!k1ii->supports_pc_live_view);
		CHECK (!k1ii->supports_cross_process);
		CHECK (k1ii->supports_card_writing_mode);
		CHECK (k1ii->supports_writing_file_format);
		CHECK (!k1ii->supports_color_temperature);
		CHECK (!k1ii->supports_ci_detail);

		CHECK (k1 && !k1->k3iii_family);
		CHECK (k1->focus_family == PENTAX_FOCUS_FAMILY_OLD);
		CHECK (k1->supports_dual_slot);
		CHECK (k1->supports_new_transfer);
		CHECK (k1->supports_movie);
		CHECK (k1->uses_it2_direct_live_view_start);
		CHECK (!k1->supports_movie_sub_image);
		CHECK (!k1->supports_tv_bracket);
		CHECK (!k1->supports_av_bracket);
		CHECK (!k1->supports_exp_bracket);
		CHECK (!k1->supports_composition_adjust);
		CHECK (!k1->supports_movie_setting);
		CHECK (!k1->supports_pc_live_view);
		CHECK (!k1->supports_cross_process);
		CHECK (k1->supports_card_writing_mode);
		CHECK (k1->supports_writing_file_format);
		CHECK (!k1->supports_color_temperature);
		CHECK (!k1->supports_ci_detail);
		/* The original K-1 and K-1 II must share the same IT2 control
		 * contract.  Their model numbers and USB identities remain distinct,
		 * but no accidental per-body fork is permitted here. */
		CHECK (k1->focus_family == k1ii->focus_family);
		CHECK (k1->supports_dual_slot == k1ii->supports_dual_slot);
		CHECK (k1->supports_new_transfer == k1ii->supports_new_transfer);
		CHECK (k1->supports_movie == k1ii->supports_movie);
		CHECK (k1->supports_movie_sub_image == k1ii->supports_movie_sub_image);
		CHECK (k1->supports_tv_bracket == k1ii->supports_tv_bracket);
		CHECK (k1->supports_av_bracket == k1ii->supports_av_bracket);
		CHECK (k1->k3iii_family == k1ii->k3iii_family);
		CHECK (k1->supports_exp_bracket == k1ii->supports_exp_bracket);
		CHECK (k1->supports_composition_adjust ==
			k1ii->supports_composition_adjust);
		CHECK (k1->supports_movie_setting == k1ii->supports_movie_setting);
		CHECK (k1->supports_aperture_keep == k1ii->supports_aperture_keep);
		CHECK (k1->supports_pc_live_view == k1ii->supports_pc_live_view);
		CHECK (k1->uses_it2_direct_live_view_start ==
			k1ii->uses_it2_direct_live_view_start);
		CHECK (k1->supports_cross_process == k1ii->supports_cross_process);
		CHECK (k1->supports_card_writing_mode ==
			k1ii->supports_card_writing_mode);
		CHECK (k1->supports_writing_file_format ==
			k1ii->supports_writing_file_format);
		CHECK (k1->supports_color_temperature ==
			k1ii->supports_color_temperature);
		CHECK (k1->supports_ci_detail == k1ii->supports_ci_detail);
		CHECK (k1->supports_self_timer_continuous_new ==
			k1ii->supports_self_timer_continuous_new);
		CHECK (k1->supports_remote_continuous_new ==
			k1ii->supports_remote_continuous_new);
		CHECK (k1->supports_slot_change == k1ii->supports_slot_change);
		CHECK (k1->is_monochrome == k1ii->is_monochrome);

		CHECK (k3 && k3->k3iii_family);
		CHECK (k3->focus_family == PENTAX_FOCUS_FAMILY_NEW);
		CHECK (k3->supports_dual_slot);
		CHECK (k3->supports_new_transfer);
		CHECK (k3->supports_movie);
		CHECK (k3->supports_movie_sub_image);
		CHECK (k3->supports_tv_bracket);
		CHECK (k3->supports_av_bracket);
		CHECK (k3->supports_exp_bracket);
		CHECK (k3->supports_composition_adjust);
		CHECK (k3->supports_movie_setting);
		CHECK (k3->supports_pc_live_view);
		CHECK (k3->supports_cross_process);
		CHECK (k3->supports_card_writing_mode);
		CHECK (k3->supports_writing_file_format);
		CHECK (k3->supports_color_temperature);
		CHECK (k3->supports_ci_detail);
		CHECK (k3->supports_self_timer_continuous_new);
		CHECK (k3->supports_remote_continuous_new);
		CHECK (k3->supports_slot_change);
		CHECK (!k3->is_monochrome);

		/* IT2 2625's explicit Monochrome gate is almost the same as the
		 * colour K-3 III, except for the product-specific colour-temperature
		 * control and its identity. */
		CHECK (mono && mono->k3iii_family);
		CHECK (mono->focus_family == PENTAX_FOCUS_FAMILY_NEW);
		CHECK (mono->supports_dual_slot);
		CHECK (mono->supports_new_transfer);
		CHECK (mono->supports_movie);
		CHECK (mono->supports_movie_sub_image);
		CHECK (mono->supports_tv_bracket);
		CHECK (mono->supports_av_bracket);
		CHECK (mono->supports_exp_bracket);
		CHECK (mono->supports_composition_adjust);
		CHECK (mono->supports_movie_setting);
		CHECK (mono->supports_aperture_keep);
		CHECK (mono->supports_pc_live_view);
		CHECK (!mono->supports_cross_process);
		CHECK (mono->supports_card_writing_mode);
		CHECK (mono->supports_writing_file_format);
		CHECK (!mono->supports_color_temperature);
		CHECK (mono->supports_ci_detail);
		CHECK (mono->supports_self_timer_continuous_new);
		CHECK (mono->supports_remote_continuous_new);
		CHECK (mono->supports_slot_change);
		CHECK (mono->is_monochrome);
	}
	CHECK (pentax_model_capabilities (0) == NULL);
	/* The later IT2 2625 decompile has a specific Monochrome branch before
	 * the colour K-3 III prefix: model 78550, new focus, and all shared K-3 III
	 * controls except colour temperature. */
	CHECK (pentax_lookup_model (0x25fb, 0x018f,
		"PENTAX K-3 Mark III Monochrome", &model_no, &extension_version));
	CHECK ((model_no == PENTAX_MODEL_K3_MARK_III_MONOCHROME) &&
		(extension_version == 1));
	/* Test the model ID that the real lookup returns. library.c stores this
	 * lookup result in params->pentax, so that is the ID used by the actual
	 * config/focus dispatch. The separate model row keeps the colour-only
	 * controls gated, while the product remains held-Bulb fail-closed until
	 * the Mono body has its own hardware qualification. */
	CHECK (pentax_model_supports_exp_bracket (model_no));
	CHECK (pentax_model_supports_composition_adjust (model_no));
	CHECK (pentax_model_supports_movie_setting (model_no));
	CHECK (pentax_model_supports_pc_live_view (model_no));
	CHECK (pentax_model_uses_new_focus (model_no));
	CHECK (!pentax_model_supports_color_temperature (model_no));
	CHECK (!pentax_bulb_action_model_supported (model_no, 0x018f));
	/* Keep the standard K-3 III product path explicit: it shares the model
	 * block and new-focus dispatch, but only its qualified PID admits the
	 * held-Bulb action. */
	CHECK (pentax_lookup_model (0x25fb, 0x0189,
		"PENTAX K-3 Mark III", &model_no, &extension_version));
	CHECK (pentax_model_uses_new_focus (model_no));
	CHECK (pentax_bulb_action_model_supported (model_no, 0x0189));
	/* K-1 II remains on the old-focus dispatch. */
	CHECK (pentax_lookup_model (0x25fb, 0x0183,
		"PENTAX K-1 Mark II", &model_no, &extension_version));
	CHECK (!pentax_model_uses_new_focus (model_no));
	CHECK (pentax_model_supports_writing_file_format (model_no));
	CHECK (pentax_model_supports_writing_file_format (PENTAX_MODEL_K1));
	CHECK (pentax_model_supports_writing_file_format (PENTAX_MODEL_K3_MARK_III));
	CHECK (!pentax_model_supports_writing_file_format (PENTAX_MODEL_K3));
	CHECK (!pentax_lookup_model (0x25fb, 0x0189,
		"PENTAX K-3 Mark III v1.9", &model_no, &extension_version));
	CHECK (!pentax_lookup_model (0x1234, 0x0189, "PENTAX K-3 Mark III",
		&model_no, &extension_version));
	/* K-5 II is a legacy USB-SCSI/MSC body.  It must not be admitted to the
	 * modern Pentax vendor/PTP path from a guessed PID or name; its existing
	 * support remains in camlibs/pentax. */
	CHECK (!pentax_lookup_model (0x25fb, 0x0148, "PENTAX K-5 II",
		&model_no, &extension_version));
	CHECK (pentax_model_capabilities (model_no) == NULL);
	memset (&unchanged, 0xa5, sizeof (unchanged));
	conditions = unchanged;
	CHECK (pentax_parse_conditions (condition_data, 507, &conditions) ==
		GP_ERROR_CORRUPTED_DATA);
	CHECK (!memcmp (&conditions, &unchanged, sizeof (conditions)));
	CHECK (pentax_parse_conditions (NULL, sizeof (condition_data), &conditions) ==
		GP_ERROR_BAD_PARAMETERS);
	CHECK (pentax_parse_conditions (condition_data, sizeof (condition_data), NULL) ==
		GP_ERROR_BAD_PARAMETERS);
	put_u32le (condition_data, 24, 49);
	put_u32le (condition_data, 40, 3);
	put_u32le (condition_data, 104, PENTAX_CONDITION_ACTIVITY_SHOOTING |
		PENTAX_CONDITION_ACTIVITY_PROCESSING);
	put_u32le (condition_data, 168, 2);
	put_u32le (condition_data, 184, 12);
	put_u32le (condition_data, 272, 300);
	put_u32le (condition_data, 276, 1);
	put_u32le (condition_data, 280, 28);
	put_u32le (condition_data, 284, 10);
	put_u32le (condition_data, 288, (uint32_t)-3);
	put_u32le (condition_data, 292, 10);
	put_u32le (condition_data, 312, 3200);
	put_u32le (condition_data, 320, PENTAX_CONDITION_ASTRO_SHIFT_MODE |
		PENTAX_CONDITION_ASTRO_MOVEMENT_FAILED |
		PENTAX_CONDITION_ASTRO_TIME_TOO_LONG);
	put_u32le (condition_data, 328, 28);
	put_u32le (condition_data, 492, 4);
	put_u32le (condition_data, 504, PENTAX_CONDITION_CAN_CHANGE_TV |
		PENTAX_CONDITION_TASK_CHANGING | PENTAX_CONDITION_BULB_TIMER |
		PENTAX_CONDITION_ASTROTRACER3 | 0x00000180U);
	put_u32le (condition_data, 528, 600);
	CHECK (pentax_parse_conditions (condition_data, 508, &conditions) == GP_OK);
	CHECK ((conditions.operation_state == 49) &&
		(conditions.activity_flags == 3) && (conditions.exposure_mode == 12) &&
		(conditions.user_mode == 3) && (conditions.exposure_step == 2) &&
		(conditions.bulb_timer_seconds == 300) &&
		(conditions.bulb_timer_denominator == 1) &&
		(conditions.aperture_numerator == 28) &&
		(conditions.aperture_denominator == 10) &&
		(conditions.exposure_comp_numerator == -3) &&
		(conditions.exposure_comp_denominator == 10) &&
		(conditions.iso == 3200) &&
		(conditions.open_av_num == 28) && (conditions.drive_mode == 4));
	CHECK ((conditions.astro_status_flags & PENTAX_CONDITION_ASTRO_TIME_TOO_LONG) &&
		(conditions.capability_flags & PENTAX_CONDITION_ASTROTRACER3) &&
		!conditions.has_astro_limit);
	CHECK (pentax_parse_conditions (condition_data, 531, &conditions) == GP_OK);
	CHECK (!conditions.has_astro_limit);
	CHECK (pentax_parse_conditions (condition_data, 532, &conditions) == GP_OK);
	CHECK (conditions.has_astro_limit && (conditions.astro_limit_seconds == 600));
	CHECK (!pentax_conditions_are_idle (&conditions));
	conditions.capability_flags &= ~PENTAX_CONDITION_TASK_CHANGING;
	conditions.activity_flags = 0;
	CHECK (pentax_conditions_are_idle (&conditions));
	CHECK (!pentax_conditions_are_idle (NULL));
	CHECK (pentax_candidate_filename (filename, sizeof (filename), name,
		sizeof (name)) == GP_OK);
	CHECK (!strcmp (name, "IMG0001.JPG"));
	CHECK (pentax_candidate_filename (filename, 8, name, sizeof (name)) ==
		GP_ERROR_CORRUPTED_DATA);
	CHECK (pentax_candidate_filename (filename, sizeof (filename), name, 4) ==
		GP_ERROR_FIXED_LIMIT_EXCEEDED);
	memcpy (malformed, filename, sizeof (filename));
	malformed[4] = '/';
	CHECK (pentax_candidate_filename (malformed, sizeof (malformed), name,
		sizeof (name)) == GP_ERROR_CORRUPTED_DATA);
	malformed[4] = 0xe9;
	CHECK (pentax_candidate_filename (malformed, sizeof (malformed), name,
		sizeof (name)) == GP_ERROR_CORRUPTED_DATA);
	{
		const unsigned char dot[] = {1, 2, 3, 1, '.', 0};
		const unsigned char dotdot[] = {1, 2, 3, 2, '.', 0, '.', 0};
		CHECK (pentax_candidate_filename (dot, sizeof (dot), name,
			sizeof (name)) == GP_ERROR_CORRUPTED_DATA);
		CHECK (pentax_candidate_filename (dotdot, sizeof (dotdot), name,
			sizeof (name)) == GP_ERROR_CORRUPTED_DATA);
	}

	/* RAW+JPEG ownership is conservative: only a different known image
	 * extension with the exact primary basename is a same-exposure companion.
	 * Same-format, differently named and unknown candidates remain stale /
	 * ambiguous and must not be consumed. */
	CHECK (pentax_candidate_is_same_exposure_companion (
		"IMG0001.JPG", "IMG0001.PEF"));
	CHECK (pentax_candidate_is_same_exposure_companion (
		"IMG0001.pef", "IMG0001.JPG"));
	CHECK (pentax_candidate_is_same_exposure_companion (
		"IMG0001.JPG", "IMG0001.DNG"));
	CHECK (!pentax_candidate_is_same_exposure_companion (
		"IMG0001.JPG", "IMG0001.JPG"));
	CHECK (!pentax_candidate_is_same_exposure_companion (
		"IMG0001.JPG", "IMG0002.PEF"));
	CHECK (!pentax_candidate_is_same_exposure_companion (
		"IMG0001.JPG", "IMG0001.TIF"));
	CHECK (!pentax_candidate_is_same_exposure_companion (
		"IMG0001", "IMG0001.PEF"));

	CHECK (pentax_capture_buffer_write (&buffer, first, sizeof (first)) == GP_OK);
	CHECK ((buffer.size == 4) && (buffer.offset == 4));
	CHECK (!memcmp (buffer.data, first, sizeof (first)));
	CHECK (pentax_capture_buffer_seek (&buffer, 4, 1) == GP_OK);
	CHECK (pentax_capture_buffer_write (&buffer, patch, sizeof (patch)) == GP_OK);
	CHECK ((buffer.size == 4) && !memcmp (buffer.data, "\x01\x09\x08\x04", 4));
	CHECK (pentax_capture_buffer_seek (&buffer, 4, 8) == GP_OK);
	CHECK (pentax_capture_buffer_write (&buffer, patch, sizeof (patch)) == GP_OK);
	CHECK ((buffer.size == 10) && (buffer.data[4] == 0) && (buffer.data[7] == 0));
	CHECK (pentax_capture_buffer_seek (&buffer, 5, -2) == GP_OK);
	CHECK (buffer.offset == 8);
	CHECK (pentax_capture_buffer_seek (&buffer, 6, -1) == GP_OK);
	CHECK (buffer.offset == 9);
	CHECK (pentax_capture_buffer_seek (&buffer, 4, -1) == GP_ERROR_BAD_PARAMETERS);
	CHECK (pentax_capture_buffer_seek (&buffer, 7, 0) == GP_ERROR_BAD_PARAMETERS);
	CHECK (pentax_capture_buffer_write (NULL, patch, sizeof (patch)) ==
		GP_ERROR_BAD_PARAMETERS);
	{
		const unsigned char framed_jpeg[] = {
			0, 1, 0xff, 0xd8, 3, 4, 0xff, 0xd9, 5
		};
		const unsigned char no_soi[] = {0, 0xff, 0xd9};
		const unsigned char no_eoi[] = {0xff, 0xd8, 1, 0xff};
		CHECK (pentax_jpeg_bounds (framed_jpeg, sizeof (framed_jpeg),
			&jpeg_offset, &jpeg_length) == GP_OK);
		CHECK ((jpeg_offset == 2) && (jpeg_length == 6));
		CHECK (pentax_jpeg_bounds (no_soi, sizeof (no_soi),
			&jpeg_offset, &jpeg_length) == GP_ERROR_CORRUPTED_DATA);
		CHECK (pentax_jpeg_bounds (no_eoi, sizeof (no_eoi),
			&jpeg_offset, &jpeg_length) == GP_ERROR_CORRUPTED_DATA);
		CHECK ((jpeg_offset == 0) && (jpeg_length == 0));
		CHECK (pentax_jpeg_bounds (NULL, 0, &jpeg_offset, &jpeg_length) ==
			GP_ERROR_BAD_PARAMETERS);
	}

	free (buffer.data);
	memset (&buffer, 0, sizeof (buffer));
	mock_init (&mock);
	mock.commands[0] = 1;
	mock.commands[1] = 3;
	mock.command_info[1] = 10;
	mock.commands[2] = 2;
	mock.command_count = 3;
	transfer_operations.user_data = &mock;
	transfer_operations.max_block_size = 4;
	transfer_operations.get_command = mock_get_command;
	transfer_operations.get_block = mock_get_block;
	transfer_operations.is_cancelled = mock_cancelled;
	transfer_operations.is_timed_out = mock_timed_out;
	CHECK (pentax_transfer_run (&buffer, &transfer_operations) == GP_OK);
	CHECK ((mock.request_count == 3) && (mock.requests[0] == 4) &&
		(mock.requests[1] == 4) && (mock.requests[2] == 2));
	CHECK ((buffer.size == 10) && !memcmp (buffer.data, "AAAABBBBCC", 10));

	free (buffer.data);
	memset (&buffer, 0, sizeof (buffer));
	mock_init (&mock);
	mock.commands[0] = 1;
	mock.commands[1] = 3;
	mock.command_info[1] = 4;
	mock.commands[2] = 4;
	mock.command_info[2] = 1;
	mock.commands[3] = 3;
	mock.command_info[3] = 2;
	mock.commands[4] = 2;
	mock.command_count = 5;
	transfer_operations.user_data = &mock;
	CHECK (pentax_transfer_run (&buffer, &transfer_operations) == GP_OK);
	CHECK ((buffer.size == 4) && !memcmp (buffer.data, "ABBA", 4));

	free (buffer.data);
	memset (&buffer, 0, sizeof (buffer));
	mock_init (&mock);
	mock.commands[0] = 2;
	mock.command_count = 1;
	transfer_operations.user_data = &mock;
	CHECK (pentax_transfer_run (&buffer, &transfer_operations) ==
		GP_ERROR_CORRUPTED_DATA);
	mock_init (&mock);
	mock.commands[0] = 1;
	mock.commands[1] = 1;
	mock.command_count = 2;
	CHECK (pentax_transfer_run (&buffer, &transfer_operations) ==
		GP_ERROR_CORRUPTED_DATA);
	mock_init (&mock);
	mock.commands[0] = 1;
	mock.commands[1] = 2;
	mock.command_count = 2;
	CHECK (pentax_transfer_run (&buffer, &transfer_operations) ==
		GP_ERROR_CORRUPTED_DATA);
	mock_init (&mock);
	mock.commands[0] = 1;
	mock.commands[1] = 9;
	mock.command_count = 2;
	CHECK (pentax_transfer_run (&buffer, &transfer_operations) ==
		GP_ERROR_NOT_SUPPORTED);
	mock_init (&mock);
	mock.command_error_at = 0;
	CHECK (pentax_transfer_run (&buffer, &transfer_operations) == GP_ERROR_IO);
	mock_init (&mock);
	mock.cancelled = 1;
	CHECK (pentax_transfer_run (&buffer, &transfer_operations) == GP_ERROR_CANCEL);
	mock_init (&mock);
	mock.timed_out = 1;
	CHECK (pentax_transfer_run (&buffer, &transfer_operations) == GP_ERROR_TIMEOUT);

	mock_init (&mock);
	mock.commands[0] = 1;
	mock.commands[1] = 3;
	mock.command_info[1] = 4;
	mock.command_count = 2;
	mock.zero_block = 1;
	CHECK (pentax_transfer_run (&buffer, &transfer_operations) ==
		GP_ERROR_CORRUPTED_DATA);
	mock_init (&mock);
	mock.commands[0] = 1;
	mock.commands[1] = 3;
	mock.command_info[1] = 4;
	mock.command_count = 2;
	mock.oversized_block = 1;
	CHECK (pentax_transfer_run (&buffer, &transfer_operations) ==
		GP_ERROR_CORRUPTED_DATA);
	mock_init (&mock);
	mock.commands[0] = 1;
	mock.commands[1] = 3;
	mock.command_info[1] = 4;
	mock.command_count = 2;
	mock.block_error_at = 0;
	CHECK (pentax_transfer_run (&buffer, &transfer_operations) == GP_ERROR_IO);

	/* Issue #35: a short block inside a declared segment must fail the
	 * transfer instead of silently truncating the image. */
	mock_init (&mock);
	mock.commands[0] = 1;
	mock.commands[1] = 3;
	mock.command_info[1] = 10;
	mock.commands[2] = 2;
	mock.command_count = 3;
	mock.short_block_at = 1;	/* second block returns 3 of 4 bytes */
	mock.short_block_bytes = 3;
	CHECK (pentax_transfer_run (&buffer, &transfer_operations) ==
		GP_ERROR_CORRUPTED_DATA);

	/* A short block that exactly satisfies the final request of a
	 * segment is legitimate and must still succeed. */
	free (buffer.data);
	memset (&buffer, 0, sizeof (buffer));
	mock_init (&mock);
	mock.commands[0] = 1;
	mock.commands[1] = 3;
	mock.command_info[1] = 6;
	mock.commands[2] = 2;
	mock.command_count = 3;
	mock.short_block_at = 1;	/* only block, full segment in one read */
	mock.short_block_bytes = 6;
	transfer_operations.max_block_size = 8;
	CHECK (pentax_transfer_run (&buffer, &transfer_operations) == GP_OK);
	CHECK ((mock.request_count == 1) && (mock.requests[0] == 6));
	CHECK ((buffer.size == 6) && !memcmp (buffer.data, "AAAAAA", 6));
	transfer_operations.max_block_size = 4;

	CHECK (pentax_transfer_run (NULL, &transfer_operations) ==
		GP_ERROR_BAD_PARAMETERS);
	/* New-focus (0x9017): positive UI direction (Near) sends a negative
	 * image-plane displacement; Far sends positive (re-verified on K-3 III
	 * real hardware 2026-09-14). */
	CHECK (pentax_minimum_focus_displacement (35, 1, &displacement) == GP_OK);
	CHECK (displacement == -29);
	CHECK (pentax_minimum_focus_displacement (35, -1, &displacement) == GP_OK);
	CHECK (displacement == 29);
	CHECK (pentax_minimum_focus_displacement (0, 1, &displacement) == GP_ERROR_CORRUPTED_DATA);
	CHECK (pentax_minimum_focus_displacement (35, 0, &displacement) == GP_ERROR_BAD_PARAMETERS);
	{
		uint32_t protocol_direction = 99;

		CHECK (pentax_old_focus_protocol_direction (1,
			&protocol_direction) == GP_OK);
		CHECK (protocol_direction == 0); /* IT2 Near */
		CHECK (pentax_old_focus_protocol_direction (-1,
			&protocol_direction) == GP_OK);
		CHECK (protocol_direction == 1); /* IT2 Far */
		CHECK (pentax_old_focus_protocol_direction (0,
			&protocol_direction) == GP_ERROR_BAD_PARAMETERS);
		CHECK (pentax_old_focus_protocol_direction (1, NULL) ==
			GP_ERROR_BAD_PARAMETERS);
	}

	/* Card writing mode (0x9004): IT2's _isDualSlot set is the K-1 and
	 * K-3 families plus 645D/645Z; KP, K-70 and GR III are single-slot. */
	CHECK (pentax_model_supports_card_writing_mode (PENTAX_MODEL_K1_MARK_II));
	CHECK (pentax_model_supports_card_writing_mode (PENTAX_MODEL_K1));
	CHECK (pentax_model_supports_card_writing_mode (PENTAX_MODEL_K3));
	CHECK (pentax_model_supports_card_writing_mode (PENTAX_MODEL_K3_MARK_III));
	CHECK (pentax_model_supports_card_writing_mode (PENTAX_MODEL_645D));
	CHECK (pentax_model_supports_card_writing_mode (PENTAX_MODEL_645Z));
	CHECK (!pentax_model_supports_card_writing_mode (PENTAX_MODEL_KP));
	CHECK (!pentax_model_supports_card_writing_mode (PENTAX_MODEL_K70));
	CHECK (!pentax_model_supports_card_writing_mode (PENTAX_MODEL_GR_III));
	CHECK (!pentax_model_supports_card_writing_mode (PENTAX_MODEL_KF));

	/* Cross process (d02c): K-1 II probe (2026-09-02) shows GET/SET
	 * unsupported even in cross-process mode, so gate on k3iii family. */
	CHECK (pentax_model_supports_cross_process (PENTAX_MODEL_K3_MARK_III));
	CHECK (!pentax_model_supports_cross_process (PENTAX_MODEL_K1_MARK_II));
	CHECK (!pentax_model_supports_cross_process (PENTAX_MODEL_K1));
	CHECK (!pentax_model_supports_cross_process (PENTAX_MODEL_K3));
	CHECK (!pentax_model_supports_cross_process (PENTAX_MODEL_KP));

	/* Research-capable PIDs (issue #19): these are the vendor bodies with an
	 * exact IMAGE Transmitter 2 model path. K-1 (0x0179) is source-authorised
	 * but remains hardware-unqualified in this fork; K-3 II (0x017b) is
	 * deliberately absent from IT2, so it must fail closed here too.
	 * 0x0182 is the K-1 II's other declared product ID and is the same body as
	 * 0x0183 (issue #179). */
	CHECK (pentax_pid_is_research_capable (0x0179)); /* K-1, hardware pending */
	CHECK (pentax_pid_is_research_capable (0x0183)); /* K-1 II */
	CHECK (!pentax_pid_is_research_capable (0x0182)); /* K-1 II MSC mode */
	CHECK (pentax_pid_is_research_capable (0x0189)); /* K-3 III */
	CHECK (pentax_pid_is_research_capable (0x018f)); /* K-3 III Mono */
	CHECK (!pentax_pid_is_research_capable (0x017b));
	CHECK (!pentax_pid_is_research_capable (0x0185));
	CHECK (!pentax_pid_is_research_capable (0));

	/* Transfer-timeout ordering (issue #38): a stalled stream must trip the
	 * short no-progress bound even when the absolute ceiling has not been
	 * reached, and a legitimately huge stream trips only the ceiling. */
	CHECK (pentax_transfer_timeout_reason (1000, 60000) ==
		PENTAX_TRANSFER_TIMEOUT_STALLED);
	CHECK (pentax_transfer_timeout_reason (1800000, 59999) ==
		PENTAX_TRANSFER_TIMEOUT_CEILING);
	CHECK (pentax_transfer_timeout_reason (1799999, 59999) ==
		PENTAX_TRANSFER_TIMEOUT_OK);
	CHECK (pentax_transfer_timeout_reason (60000, 60000) ==
		PENTAX_TRANSFER_TIMEOUT_STALLED);

	/* Capture-wait budget arithmetic: camera-reported values are untrusted
	 * protocol input, so every branch must stay in 64-bit and clamp to the
	 * absolute ceiling (issue #43 regression coverage). */
	memset (&conditions, 0, sizeof (conditions));
	CHECK (pentax_capture_timeout_ms (&conditions) ==
		PENTAX_CAPTURE_TIMEOUT_MS_BASE);

	/* The bulb timer is the exposure duration, so the budget must also carry
	 * a dark frame of about the same length again: 121s + 121s + 30s. */
	conditions.bulb_timer_seconds = 120;
	CHECK (pentax_capture_timeout_ms (&conditions) == 272000);

	memset (&conditions, 0, sizeof (conditions));
	conditions.activity_flags = PENTAX_CONDITION_ACTIVITY_MULTI_MODE;
	CHECK (pentax_capture_timeout_ms (&conditions) == 270000); /* 4x + margin */

	/* Four 10 s frames, each able to carry a dark frame: 40s + 40s + 30s. */
	conditions.bulb_timer_seconds = 9; /* bulb + multi-shot composite */
	CHECK (pentax_capture_timeout_ms (&conditions) == 110000);

	memset (&conditions, 0, sizeof (conditions));
	conditions.astro_status_flags = PENTAX_CONDITION_ASTRO_SHIFT_MODE;
	conditions.has_astro_limit = 1;
	conditions.astro_limit_seconds = 300;
	/* 301s exposure + 301s dark frame + 30s margin. */
	CHECK (pentax_capture_timeout_ms (&conditions) == 632000);

	/* Overflow-safe clamp: a corrupt UINT32_MAX limit must not wrap the
	 * 64-bit math or exceed the ceiling. */
	conditions.astro_limit_seconds = UINT32_MAX;
	CHECK (pentax_capture_timeout_ms (&conditions) ==
		PENTAX_CAPTURE_TIMEOUT_MS_MAX);

	/* Astro Tracer live mode is signalled by the exposure-mode dial value
	 * (offset 184) == ExpMode.AstroTracer (20), NOT by the offset-504
	 * capability bit.  The capability bit alone must not widen the budget:
	 * a body that CAN do astro but is currently in P mode stays at base. */
	memset (&conditions, 0, sizeof (conditions));
	conditions.capability_flags = PENTAX_CONDITION_ASTROTRACER3;
	CHECK (pentax_capture_timeout_ms (&conditions) ==
		PENTAX_CAPTURE_TIMEOUT_MS_BASE);

	/* In astro mode with no reported limit: base + margin. */
	memset (&conditions, 0, sizeof (conditions));
	conditions.exposure_mode = PENTAX_EXP_MODE_ASTROTRACER;
	CHECK (pentax_capture_timeout_ms (&conditions) == 90000); /* base + margin */

	/* In astro mode with a reported AstroTracerTimeLimit: honour the limit. */
	memset (&conditions, 0, sizeof (conditions));
	conditions.exposure_mode = PENTAX_EXP_MODE_ASTROTRACER;
	conditions.has_astro_limit = 1;
	conditions.astro_limit_seconds = 300;
	/* 301s exposure + 301s dark frame + 30s margin. */
	CHECK (pentax_capture_timeout_ms (&conditions) == 632000);

	/* Regression: both observed losses were budget-bound, not camera faults. A
	 * 70 s bulb gave up at 107.4 s against a 101 s budget and a 40 s bulb at
	 * 71.6 s against 71 s, while the exposure plus its dark frame needs about
	 * twice the exposure. The wait loop returns as soon as the candidate
	 * appears, so widening these deadlines costs nothing on a successful
	 * capture; the old flat 30 s margin discarded real photographs instead. */
	memset (&conditions, 0, sizeof (conditions));
	conditions.bulb_timer_seconds = 70;
	CHECK (pentax_capture_timeout_ms (&conditions) >= 146000); /* NR ON worst case */
	memset (&conditions, 0, sizeof (conditions));
	conditions.bulb_timer_seconds = 40;
	CHECK (pentax_capture_timeout_ms (&conditions) >= 85000);

	/* Offsets 272/276 are the current shutter speed in EVERY mode, not a
	 * bulb-only field: IT2 reads that pair as the live Tv value and decodes it
	 * with ConvertFractionalTvValueToString (den==1 -> "N\"", num==1 -> "1/D",
	 * else the quotient), and the shutter-verify path in config.c compares the
	 * requested shutter against the same two words.  The two assertions above
	 * therefore already cover a non-bulb long exposure: they set only the Tv
	 * fields and no bulb mode, which is exactly what a timed-mode 40 s capture
	 * reports.  Observed on hardware: a request carrying no bulb field at all
	 * gave up at 71 s, and under the pre-23008c229 code (bts+1)+30000 = 71000
	 * is the only branch that yields that, so offset 272 held 40 while the dial
	 * was on a timed shutter.
	 *
	 * A fractional shutter must not inflate the budget.  1/200 s reports
	 * numerator 1, so the scaled allowance is 2s + 2s + 30s = 34s, below the
	 * 60s base, and the base dominates.  Sub-second values can only over-size,
	 * never under-size, which is the cheap direction. */
	memset (&conditions, 0, sizeof (conditions));
	conditions.bulb_timer_seconds = 1;
	conditions.bulb_timer_denominator = 200;
	CHECK (pentax_capture_timeout_ms (&conditions) ==
		PENTAX_CAPTURE_TIMEOUT_MS_BASE);

	/* A whole-second shutter below the base likewise leaves the base in charge:
	 * 13s + 13s + 30s = 56s < 60s. */
	memset (&conditions, 0, sizeof (conditions));
	conditions.bulb_timer_seconds = 13;
	conditions.bulb_timer_denominator = 1;
	CHECK (pentax_capture_timeout_ms (&conditions) ==
		PENTAX_CAPTURE_TIMEOUT_MS_BASE);

	/* A long timed shutter is scaled exactly as a bulb would be, because it is
	 * the same field: 41s + 41s + 30s. */
	memset (&conditions, 0, sizeof (conditions));
	conditions.bulb_timer_seconds = 40;
	conditions.bulb_timer_denominator = 1;
	CHECK (pentax_capture_timeout_ms (&conditions) == 112000);

	/* A proxy must not be scaled. With no reported exposure duration the base
	 * timeout is only a stand-in, so the flat margin still applies and a short
	 * pixel-shift composite is not widened for an exposure nobody measured. */
	memset (&conditions, 0, sizeof (conditions));
	conditions.activity_flags = PENTAX_CONDITION_ACTIVITY_MULTI_MODE;
	CHECK (pentax_capture_timeout_ms (&conditions) == 270000); /* 4x base + margin */
	memset (&conditions, 0, sizeof (conditions));
	conditions.exposure_mode = PENTAX_EXP_MODE_ASTROTRACER;
	CHECK (pentax_capture_timeout_ms (&conditions) == 90000); /* base + margin */

	/* Exposure-phase budget (issue #111): the time the camera needs to
	 * complete the exposure itself, before post-exposure processing.  This is
	 * deliberately separate from pentax_capture_timeout_ms() so a long Bulb
	 * does not consume its entire all-in budget before RAW processing starts. */
	memset (&conditions, 0, sizeof (conditions));
	CHECK (pentax_exposure_phase_ms (&conditions) ==
		PENTAX_CAPTURE_TIMEOUT_MS_BASE);

	conditions.bulb_timer_seconds = 120; /* 121s exposure phase, no margin */
	CHECK (pentax_exposure_phase_ms (&conditions) == 121000);

	memset (&conditions, 0, sizeof (conditions));
	conditions.activity_flags = PENTAX_CONDITION_ACTIVITY_MULTI_MODE;
	CHECK (pentax_exposure_phase_ms (&conditions) == 240000); /* 4x base */

	conditions.bulb_timer_seconds = 9; /* bulb + multi-shot composite:
	 * 10s*4=40s < base 60s, so the base dominates */
	CHECK (pentax_exposure_phase_ms (&conditions) == 60000);

	/* A long enough bulb timer that the multi-shot composite exceeds the
	 * base: 30s bulb * 4 = 120s > 60s base. */
	conditions.bulb_timer_seconds = 30;
	CHECK (pentax_exposure_phase_ms (&conditions) == 124000); /* 31s*4 */

	/* Astro Tracer exposure phase: honour the camera's own AstroTracerTimeLimit
	 * so a long astro exposure is not aborted before the shutter closes.  The
	 * capability bit alone (body CAN do astro) must not widen the phase. */
	memset (&conditions, 0, sizeof (conditions));
	conditions.capability_flags = PENTAX_CONDITION_ASTROTRACER3;
	CHECK (pentax_exposure_phase_ms (&conditions) ==
		PENTAX_CAPTURE_TIMEOUT_MS_BASE);

	memset (&conditions, 0, sizeof (conditions));
	conditions.exposure_mode = PENTAX_EXP_MODE_ASTROTRACER;
	conditions.has_astro_limit = 1;
	conditions.astro_limit_seconds = 300; /* 301s exposure phase */
	CHECK (pentax_exposure_phase_ms (&conditions) == 301000);

	/* In astro mode with no reported limit: the base dominates. */
	memset (&conditions, 0, sizeof (conditions));
	conditions.exposure_mode = PENTAX_EXP_MODE_ASTROTRACER;
	CHECK (pentax_exposure_phase_ms (&conditions) ==
		PENTAX_CAPTURE_TIMEOUT_MS_BASE);

	/* Overflow-safe clamp: a corrupt UINT32_MAX bulb timer must not wrap the
	 * 64-bit math or exceed the ceiling. */
	memset (&conditions, 0, sizeof (conditions));
	conditions.bulb_timer_seconds = UINT32_MAX;
	CHECK (pentax_exposure_phase_ms (&conditions) ==
		PENTAX_CAPTURE_TIMEOUT_MS_MAX);

	/* Timeout phase attribution uses the camera's last +104 state, not the
	 * necessarily-zero candidate handle at the timeout boundary. */
	CHECK (pentax_capture_timeout_state (0) ==
		PENTAX_CAPTURE_TIMEOUT_STATE_UNKNOWN);
	CHECK (pentax_capture_timeout_state (PENTAX_CONDITION_ACTIVITY_SHOOTING) ==
		PENTAX_CAPTURE_TIMEOUT_STATE_EXPOSING);
	CHECK (pentax_capture_timeout_state (PENTAX_CONDITION_ACTIVITY_PROCESSING) ==
		PENTAX_CAPTURE_TIMEOUT_STATE_PROCESSING);
	CHECK (pentax_capture_timeout_state (PENTAX_CONDITION_ACTIVITY_SHOOTING |
		PENTAX_CONDITION_ACTIVITY_PROCESSING) ==
		PENTAX_CAPTURE_TIMEOUT_STATE_PROCESSING);

	/* Session reconciliation decisions (issue #33): short blobs and unsafe
	 * activity force recovery, a pending candidate must be surfaced. The
	 * out-param is only written on STALE_CANDIDATE; sentinel checks pin that. */
	memset (condition_data, 0, sizeof (condition_data));
	uint32_t candidate = 99;
	CHECK (pentax_reconcile_conditions (NULL, 0, &candidate) ==
		PENTAX_RECONCILE_UNREADABLE);
	CHECK (pentax_reconcile_conditions (condition_data, 507, &candidate) ==
		PENTAX_RECONCILE_UNREADABLE);

	put_u32le (condition_data, 104, PENTAX_CONDITION_ACTIVITY_SHOOTING);
	CHECK (pentax_reconcile_conditions (condition_data, sizeof (condition_data),
			&candidate) == PENTAX_RECONCILE_UNSAFE);
	CHECK (candidate == 99); /* untouched on UNSAFE */

	memset (condition_data, 0, sizeof (condition_data));
	put_u32le (condition_data, 36, 42);
	candidate = 0;
	CHECK (pentax_reconcile_conditions (condition_data, sizeof (condition_data),
			&candidate) == PENTAX_RECONCILE_STALE_CANDIDATE);
	CHECK (candidate == 42);

	memset (condition_data, 0, sizeof (condition_data));
	candidate = 99;
	CHECK (pentax_reconcile_conditions (condition_data, sizeof (condition_data),
			&candidate) == PENTAX_RECONCILE_IDLE);
	CHECK (candidate == 99); /* untouched on IDLE */

	put_u32le (condition_data, 36, 7);
	CHECK (pentax_reconcile_conditions (condition_data, sizeof (condition_data),
			NULL) == PENTAX_RECONCILE_STALE_CANDIDATE);

	/* Recovery re-probe predicate: only a complete, idle, non-capturing
	 * blob is acceptable; field 32 == 1 means capture in progress. */
	CHECK (!pentax_recovery_probe_ok (NULL, 0));
	memset (condition_data, 0, sizeof (condition_data));
	CHECK (!pentax_recovery_probe_ok (condition_data, 507));
	put_u32le (condition_data, 104, PENTAX_CONDITION_ACTIVITY_PROCESSING);
	CHECK (!pentax_recovery_probe_ok (condition_data, sizeof (condition_data)));
	memset (condition_data, 0, sizeof (condition_data));
	put_u32le (condition_data, 32, 1);
	CHECK (!pentax_recovery_probe_ok (condition_data, sizeof (condition_data)));
	put_u32le (condition_data, 32, 5);
	put_u32le (condition_data, 36, 7);
	CHECK (!pentax_recovery_probe_ok (condition_data, sizeof (condition_data)));
	put_u32le (condition_data, 36, 0);
	CHECK (pentax_recovery_probe_ok (condition_data, sizeof (condition_data)));

	/* The diagnostic output-safe policy differs from strict policy only on
	 * broad activity: neither policy may admit unreadable conditions, an
	 * active exposure, or a pending output candidate. */
	put_u32le (condition_data, 104, PENTAX_CONDITION_ACTIVITY_PROCESSING);
	CHECK (!pentax_admission_probe_ok (condition_data, sizeof (condition_data),
		PENTAX_ADMISSION_STRICT));
	CHECK (pentax_admission_probe_ok (condition_data, sizeof (condition_data),
		PENTAX_ADMISSION_OUTPUT_SAFE));
	/* Output-safe is telemetry for hardware comparison. The strict result
	 * above remains the authoritative permission to issue another shutter. */
	put_u32le (condition_data, 32, 1);
	CHECK (!pentax_admission_probe_ok (condition_data, sizeof (condition_data),
		PENTAX_ADMISSION_OUTPUT_SAFE));
	put_u32le (condition_data, 32, 0);
	put_u32le (condition_data, 36, 9);
	CHECK (!pentax_admission_probe_ok (condition_data, sizeof (condition_data),
		PENTAX_ADMISSION_STRICT));
	CHECK (!pentax_admission_probe_ok (condition_data, sizeof (condition_data),
		PENTAX_ADMISSION_OUTPUT_SAFE));
	put_u32le (condition_data, 36, 0);
	put_u32le (condition_data, 104, 0);
	CHECK (pentax_admission_probe_ok (condition_data, sizeof (condition_data),
		PENTAX_ADMISSION_STRICT));

	/* Every fail-closed admission cause must be separately diagnosable and
	 * carry a non-destructive recovery action. */
	CHECK (pentax_admission_block_reason (NULL, 0,
		PENTAX_ADMISSION_STRICT) == PENTAX_ADMISSION_BLOCK_UNREADABLE);
	memset (condition_data, 0, sizeof (condition_data));
	put_u32le (condition_data, 104, PENTAX_CONDITION_ACTIVITY_PROCESSING);
	CHECK (pentax_admission_block_reason (condition_data, sizeof (condition_data),
		PENTAX_ADMISSION_STRICT) == PENTAX_ADMISSION_BLOCK_UNSAFE_ACTIVITY);
	CHECK (pentax_admission_block_reason (condition_data, sizeof (condition_data),
		PENTAX_ADMISSION_OUTPUT_SAFE) == PENTAX_ADMISSION_BLOCK_NONE);
	put_u32le (condition_data, 104, 0);
	put_u32le (condition_data, 32, 1);
	CHECK (pentax_admission_block_reason (condition_data, sizeof (condition_data),
		PENTAX_ADMISSION_STRICT) == PENTAX_ADMISSION_BLOCK_TRANSFER_CANDIDATE_AVAILABLE);
	put_u32le (condition_data, 32, 0);
	put_u32le (condition_data, 36, 23);
	CHECK (pentax_admission_block_reason (condition_data, sizeof (condition_data),
		PENTAX_ADMISSION_STRICT) == PENTAX_ADMISSION_BLOCK_SELECTOR_PRESENT);
	CHECK (!strcmp (pentax_admission_block_reason_name (
		PENTAX_ADMISSION_BLOCK_SELECTOR_PRESENT), "selector-present"));
	CHECK (strstr (pentax_admission_recovery_action (
		PENTAX_ADMISSION_BLOCK_SELECTOR_PRESENT), "recover-output-with-ownership") != NULL);
	CHECK (strstr (pentax_admission_recovery_action (
		PENTAX_ADMISSION_BLOCK_UNSAFE_ACTIVITY), "without-timed-retry") != NULL);
	CHECK (!strcmp (pentax_admission_block_reason_name (
		PENTAX_ADMISSION_BLOCK_NONE), "none"));

	/* Camera readiness can discharge a recovery flag only when there is no
	 * accepted capture whose output obligation remains unresolved.  This is the
	 * production recovery decision used before another InitiateCapture. */
	CHECK (pentax_recovery_probe_can_clear (
		PENTAX_ADMISSION_BLOCK_NONE, 0));
	CHECK (!pentax_recovery_probe_can_clear (
		PENTAX_ADMISSION_BLOCK_NONE, 1));
	CHECK (!pentax_recovery_probe_can_clear (
		PENTAX_ADMISSION_BLOCK_OUTPUT_UNRESOLVED, 1));
	CHECK (!pentax_recovery_probe_can_clear (
		PENTAX_ADMISSION_BLOCK_UNSAFE_ACTIVITY, 0));

	/* A latched output obligation is our own bookkeeping, so it cannot
	 * discharge itself: an abandoned capture never reaches the publication
	 * that clears it. The camera reporting a readable idle body with no
	 * active exposure and no candidate is the positive contradiction that
	 * releases it (Clog_000250 on o-v15r: captures 5 and 6 refused with
	 * field32=0x00000000 field36=0x00000000 after capture 4 timed out at
	 * 66 s, and every later shutter was locked out until restart). */
	CHECK (pentax_output_obligation_releasable (
		PENTAX_ADMISSION_BLOCK_NONE, 0, 0, 0));
	/* A capture we are still serving owns whatever the camera might owe. */
	CHECK (!pentax_output_obligation_releasable (
		PENTAX_ADMISSION_BLOCK_NONE, 1, 0, 0));
	/* Unreadable conditions are unknown, not empty. */
	CHECK (!pentax_output_obligation_releasable (
		PENTAX_ADMISSION_BLOCK_UNREADABLE, 0, 0, 0));
	/* An active camera operation may still produce the output. */
	CHECK (!pentax_output_obligation_releasable (
		PENTAX_ADMISSION_BLOCK_UNSAFE_ACTIVITY, 0, 0, 0));
	/* An exposure in progress, or a frame still on the body, is a real
	 * obligation (or an orphan to claim), never a stale flag. */
	CHECK (!pentax_output_obligation_releasable (
		PENTAX_ADMISSION_BLOCK_NONE, 0, 1, 0));
	CHECK (!pentax_output_obligation_releasable (
		PENTAX_ADMISSION_BLOCK_NONE, 0, 0, 9));
	/* Only the explicit zero the camera reports counts; the block reason
	 * singles out particular non-zero values, so do not infer from it. */
	CHECK (!pentax_output_obligation_releasable (
		PENTAX_ADMISSION_BLOCK_NONE, 0, 2, 0));
	/* Releasing the flag must not be confused with claiming a candidate:
	 * with a handle present the orphan path still owns the decision. */
	CHECK (!pentax_orphan_candidate_claimable (
		PENTAX_ADMISSION_BLOCK_OUTPUT_UNRESOLVED, 0, 9));

	/* Issue #175: "recover-output-with-ownership" must have a decidable
	 * trigger. A candidate-bearing block with no capture of ours in flight is
	 * an orphan we may claim; anything else keeps the fail-closed refusal. */
	CHECK (pentax_orphan_candidate_claimable (
		PENTAX_ADMISSION_BLOCK_TRANSFER_CANDIDATE_AVAILABLE, 0, 9));
	CHECK (pentax_orphan_candidate_claimable (
		PENTAX_ADMISSION_BLOCK_SELECTOR_PRESENT, 0, 9));
	/* Our own accepted exposure still owns the object: never claim it here. */
	CHECK (!pentax_orphan_candidate_claimable (
		PENTAX_ADMISSION_BLOCK_TRANSFER_CANDIDATE_AVAILABLE, 1, 9));
	/* No handle means there is nothing to claim. */
	CHECK (!pentax_orphan_candidate_claimable (
		PENTAX_ADMISSION_BLOCK_TRANSFER_CANDIDATE_AVAILABLE, 0, 0));
	/* Unreadable conditions and an active camera operation are not orphans;
	 * claiming them would issue PTP traffic at a busy body. */
	CHECK (!pentax_orphan_candidate_claimable (
		PENTAX_ADMISSION_BLOCK_UNREADABLE, 0, 9));
	CHECK (!pentax_orphan_candidate_claimable (
		PENTAX_ADMISSION_BLOCK_UNSAFE_ACTIVITY, 0, 9));
	/* OUTPUT_UNRESOLVED is derived only from our own pending flag. */
	CHECK (!pentax_orphan_candidate_claimable (
		PENTAX_ADMISSION_BLOCK_OUTPUT_UNRESOLVED, 0, 9));
	CHECK (!pentax_orphan_candidate_claimable (
		PENTAX_ADMISSION_BLOCK_NONE, 0, 9));

	/* Issue #176 follow-up: a claimed orphan must reach disk, not just the
	 * in-memory publication list that the caller clears before returning.
	 * The device claim at 21:22:59 logged recovered=1 names=IMGP3794.DNG and
	 * no such file existed anywhere on the card. */
	{
		char dest[512];

		/* The extension stays last so the recovered file is still openable. */
		CHECK (pentax_orphan_recovery_path ("/app/sd/normal", "IMGP3794.DNG",
			233, dest, sizeof (dest)));
		CHECK (!strcmp (dest, "/app/sd/normal/IMGP3794-orphan-233.DNG"));
		/* Only the final component of a camera-side name is used, so a hostile
		 * name cannot escape the recovery directory. */
		CHECK (pentax_orphan_recovery_path ("/app/sd/normal",
			"../../etc/passwd", 1, dest, sizeof (dest)));
		CHECK (!strncmp (dest, "/app/sd/normal/passwd-orphan-1", 32));
		CHECK (pentax_orphan_recovery_path ("/app/sd/normal",
			"/store_00010001/DCIM/489_1005/IMGP3794.JPG", 7, dest,
			sizeof (dest)));
		CHECK (!strcmp (dest,
			"/app/sd/normal/IMGP3794-orphan-7.JPG"));
		/* A name that sanitises away entirely still yields a distinct file. */
		CHECK (pentax_orphan_recovery_path ("/app/sd/normal", "...", 5, dest,
			sizeof (dest)));
		CHECK (!strcmp (dest, "/app/sd/normal/unknown-orphan-5"));
		/* Empty/unset directory => no write, previous behaviour preserved. */
		CHECK (!pentax_orphan_recovery_path ("", "IMGP1.DNG", 1, dest,
			sizeof (dest)));
		CHECK (!pentax_orphan_recovery_path (NULL, "IMGP1.DNG", 1, dest,
			sizeof (dest)));
		/* Relative directories are refused: the write target must be
		 * unambiguous regardless of the process cwd. */
		CHECK (!pentax_orphan_recovery_path ("app/sd/normal", "IMGP1.DNG", 1,
			dest, sizeof (dest)));
		/* A buffer too small to hold the path is a refusal, not a truncation. */
		CHECK (!pentax_orphan_recovery_path ("/app/sd/normal",
			"IMGP3794.DNG", 233, dest, 16));
	}

	/* Issue #176 review: "we fixed recovering-the-orphan-throwing-the-
	 * photograph-away; configuration must not be able to switch that bug
	 * back on." A camera object may be deleted if and only if a durable
	 * owner has been positively established. */
	{
		char first[512];
		char second[512];

		/* 1. Durable destination disabled/empty -> no camera delete. */
		CHECK (!pentax_orphan_recovery_path ("", "IMGP3794.DNG", 9, first,
			sizeof (first)));
		CHECK (!pentax_orphan_delete_permitted (1, 0));
		/* 2. Durable save fails -> no camera delete. The write is what
		 * sets the evidence, so a failure leaves it exactly as above. */
		CHECK (!pentax_orphan_delete_permitted (1, 0));
		/* 3. Durable save succeeds -> delete permitted. */
		CHECK (pentax_orphan_recovery_path ("/app/sd/normal",
			"IMGP3794.DNG", 9, first, sizeof (first)));
		CHECK (pentax_orphan_delete_permitted (1, 1));
		/* 4. Destination collision -> a unique durable file, then delete.
		 * The earlier recovered frame must not be overwritten. */
		CHECK (pentax_orphan_recovery_path_unique ("/app/sd/normal",
			"IMGP3794.DNG", 9, 0, first, sizeof (first)));
		CHECK (pentax_orphan_recovery_path_unique ("/app/sd/normal",
			"IMGP3794.DNG", 9, 1, second, sizeof (second)));
		CHECK (strcmp (first, second) != 0);
		/* Still openable: the extension stays last. */
		CHECK (strlen (second) > 4 &&
			!strcmp (second + strlen (second) - 4, ".DNG"));
		CHECK (!strncmp (second, "/app/sd/normal/IMGP3794-orphan-9", 32));
		/* A refused destination is never a silent overwrite either. */
		CHECK (!pentax_orphan_recovery_path_unique ("", "IMGP3794.DNG", 9, 1,
			second, sizeof (second)));
		CHECK (!pentax_orphan_recovery_path_unique ("/app/sd/normal",
			"IMGP3794.DNG", 9, 0, second, 16));
		/* 5. No Polaris-specific environment configured -> safe, not
		 * silently in-memory-only: no destination means no delete. */
		CHECK (!pentax_orphan_recovery_path (NULL, "IMGP3794.DNG", 9, first,
			sizeof (first)));
		CHECK (!pentax_orphan_delete_permitted (1, 0));
		/* The dual-format path is not an orphan claim: the bytes go to a
		 * live caller, so it must keep working without a recovery dir. */
		CHECK (pentax_orphan_delete_permitted (0, 0));
	}

	CHECK (!pentax_capture_output_obligation_resolved (1, 0, 0));
	CHECK (!pentax_capture_output_obligation_resolved (1, 1, 0));
	CHECK (pentax_capture_output_obligation_resolved (1, 1, 1));
	CHECK (pentax_capture_output_obligation_resolved (0, 0, 0));
	CHECK (!pentax_candidate_output_published (0, 1, 1));
	CHECK (!pentax_candidate_output_published (1, 0, 1));
	CHECK (!pentax_candidate_output_published (1, 1, 0));
	CHECK (pentax_candidate_output_published (1, 1, 1));
	CHECK (!pentax_capture_initiate_response_ambiguous (0x2001));
	CHECK (!pentax_capture_initiate_response_ambiguous (0x2019));
	CHECK (pentax_capture_initiate_response_ambiguous (0x02f9));
	CHECK (pentax_capture_initiate_response_ambiguous (0x02fa));
	CHECK (pentax_capture_initiate_response_ambiguous (0x02fb));
	CHECK (pentax_capture_initiate_response_ambiguous (0x02ff));
	CHECK (!pentax_capture_initiate_response_ambiguous (0x0300));
	CHECK (!strcmp (pentax_admission_block_reason_name (
		PENTAX_ADMISSION_BLOCK_OUTPUT_UNRESOLVED),
		"output-obligation-unresolved"));
	CHECK (strstr (pentax_admission_recovery_action (
		PENTAX_ADMISSION_BLOCK_OUTPUT_UNRESOLVED),
		"keep-shutter-blocked") != NULL);
	/* A short/unknown format must not silently become a one-file capture. */
	CHECK (!pentax_capture_output_contract_known (NULL, 0));
	CHECK (!pentax_capture_output_contract_known (condition_data, 527));
	for (i = 0; i <= PTP_PENTAX_WRITING_FORMAT_TIFF; i++) {
		put_u32le (condition_data, 524, i);
		CHECK (pentax_capture_output_contract_known (condition_data,
			sizeof (condition_data)));
		CHECK (pentax_expected_extra_candidates (condition_data,
			sizeof (condition_data)) == (i ==
			PTP_PENTAX_WRITING_FORMAT_RAW_JPEG ? 1U : 0U));
	}
	put_u32le (condition_data, 524, 6);
	CHECK (!pentax_capture_output_contract_known (condition_data,
		sizeof (condition_data)));

	/* Stale-candidate baseline (issue #34): the pending transfer handle is
	 * only valid when the blob is complete and field 32 flags an active
	 * capture; otherwise callers proceed without the check. */
	CHECK (pentax_stale_candidate_baseline (NULL, 0) == 0);
	memset (condition_data, 0, sizeof (condition_data));
	put_u32le (condition_data, 36, 77);
	CHECK (pentax_stale_candidate_baseline (condition_data, sizeof (condition_data)) == 0);
	put_u32le (condition_data, 32, 1);
	CHECK (pentax_stale_candidate_baseline (condition_data, sizeof (condition_data)) == 77);

	/* Shutter-speed display formatting: whole-second timer values (Bulb
	 * timer, denominator 1) must preserve total seconds in the Benro
	 * compatible MM:SS spelling.  The old minute-bearing labels (1m10s)
	 * were parsed by the proprietary UI as just 60 seconds. */
	{
		char fmt[64];
		CHECK (pentax_format_shutter_speed (0, fmt, sizeof (fmt)) == 0);
		CHECK (!strcmp (fmt, "Auto"));
		/* The minute boundaries must retain the remainder end-to-end. */
		{
			static const uint32_t durations[] = {23, 59, 60, 61, 110, 119, 120, 121};
			static const char *labels[] = {
				"00:23", "00:59", "01:00", "01:01", "01:50",
				"01:59", "02:00", "02:01"
			};
			size_t i;
			for (i = 0; i < sizeof (durations) / sizeof (durations[0]); i++) {
				CHECK (pentax_format_shutter_speed (((uint64_t)1 << 32) |
					durations[i], fmt, sizeof (fmt)) == 0);
				CHECK (!strcmp (fmt, labels[i]));
				CHECK (pentax_parse_shutter_duration (fmt, &candidate) == 0);
				CHECK (candidate == durations[i]);
			}
		}
		/* Existing unit-bearing values remain accepted on input only. */
		CHECK (pentax_parse_shutter_duration ("30s", &candidate) == 0);
		CHECK (candidate == 30);
		CHECK (pentax_parse_shutter_duration ("1m10s", &candidate) == 0);
		CHECK (candidate == 70);
		CHECK (pentax_parse_shutter_duration ("1m", &candidate) == 0);
		CHECK (candidate == 60);
		CHECK (pentax_parse_shutter_duration ("1:10", &candidate) == 0);
		CHECK (candidate == 70);
		CHECK (pentax_parse_shutter_duration ("1m60s", &candidate) == -1);
		CHECK (pentax_parse_shutter_duration ("01:60", &candidate) == -1);
		CHECK (pentax_parse_shutter_duration ("4294967296s", &candidate) == -1);
		CHECK (pentax_parse_shutter_duration ("-18446744073709551615s", &candidate) == -1);
		CHECK (pentax_parse_shutter_duration ("1m-1s", &candidate) == -1);
		/* A rejected unit-bearing duration must not be accepted by the
		 * legacy plain-integer fallback as its leading numeric prefix. */
		CHECK (pentax_parse_legacy_shutter_denominator ("1m60s", &candidate) == -1);
		CHECK (pentax_parse_legacy_shutter_denominator ("4294967296s", &candidate) == -1);
		CHECK (pentax_parse_legacy_shutter_denominator ("-18446744073709551615", &candidate) == -1);
		CHECK (pentax_parse_legacy_shutter_denominator ("30junk", &candidate) == -1);
		/* Preserve the historical plain-integer spelling: 30 means 1/30 s. */
		CHECK (pentax_parse_legacy_shutter_denominator ("30", &candidate) == 0);
		CHECK (candidate == 30);
		/* 1/60 s: low 32 = 1, high 32 = 60. */
		CHECK (pentax_format_shutter_speed (((uint64_t)60 << 32) | 1ULL, fmt, sizeof (fmt)) == 0);
		CHECK (!strcmp (fmt, "1/60"));
		/* 1/2 s: low 32 = 1, high 32 = 2. */
		CHECK (pentax_format_shutter_speed (((uint64_t)2 << 32) | 1ULL, fmt, sizeof (fmt)) == 0);
		CHECK (!strcmp (fmt, "1/2"));
		/* 2/3 s: low 32 = 2, high 32 = 3. */
		CHECK (pentax_format_shutter_speed (((uint64_t)3 << 32) | 2ULL, fmt, sizeof (fmt)) == 0);
		CHECK (!strcmp (fmt, "2/3"));
		/* Too-small buffer is rejected. */
		CHECK (pentax_format_shutter_speed (((uint64_t)1 << 32) | 120ULL, fmt, 3) == -1);
	}


        /* Post-capture readiness tri-state (issue #122): only a VALID frame may
         * prove idle.  A failed/short read must classify as UNKNOWN, never IDLE
         * - treating it as idle let Benro fire the next shutter while the camera
         * was still busy and wedge it until a mode toggle + USB reset. */
        CHECK (pentax_camera_readiness (NULL, 0) == PENTAX_READINESS_UNKNOWN);
        CHECK (pentax_camera_readiness (condition_data, PENTAX_CONDITIONS_MIN_SIZE - 1)
               == PENTAX_READINESS_UNKNOWN);
        memset (condition_data, 0, sizeof (condition_data));
        /* Valid frame, no activity, no pending candidate -> IDLE. */
        CHECK (pentax_camera_readiness (condition_data, sizeof (condition_data))
               == PENTAX_READINESS_IDLE);
        /* Busy: unsafe activity flag set. */
        put_u32le (condition_data, 104, PENTAX_CONDITION_ACTIVITY_SHOOTING);
        CHECK (pentax_camera_readiness (condition_data, sizeof (condition_data))
               == PENTAX_READINESS_BUSY);
        memset (condition_data, 0, sizeof (condition_data));
        /* Busy: pending transfer candidate. */
        put_u32le (condition_data, 36, 77);
        CHECK (pentax_camera_readiness (condition_data, sizeof (condition_data))
               == PENTAX_READINESS_BUSY);

        /* Issue #122 (TA follow-up): fail-closed readiness wait.  Only a
         * POSITIVE IDLE transition may complete; bound exhaustion on BUSY or
         * UNKNOWN must report "not idle" so the caller returns CAMERA_BUSY
         * instead of claiming a successful capture. */
        {
                struct mock_reader {
                        unsigned char frame[532];
                        int fail_read;
                        int calls;
                } mr;

                int
                mock_read_conditions (void *user_data, unsigned char **data,
                                      unsigned int *size)
                {
                        struct mock_reader *m = user_data;
                        m->calls++;
                        if (m->fail_read)
                                return 1;            /* non-PTP_RC_OK -> UNKNOWN */
                        *data = malloc (sizeof (m->frame));
                        if (*data)
                                memcpy (*data, m->frame, sizeof (m->frame));
                        *size = sizeof (m->frame);
                        return 0;                   /* PTP_RC_OK */
                }

                memset (&mr, 0, sizeof (mr));
                CHECK (pentax_wait_for_idle (mock_read_conditions, &mr, NULL, NULL, 1000) == 1);
                CHECK (mr.calls >= 1);

                memset (&mr, 0, sizeof (mr));
                put_u32le (mr.frame, 104, PENTAX_CONDITION_ACTIVITY_SHOOTING);
                CHECK (pentax_wait_for_idle (mock_read_conditions, &mr, NULL, NULL, 1000) == 0);

                memset (&mr, 0, sizeof (mr));
                mr.fail_read = 1;
                CHECK (pentax_wait_for_idle (mock_read_conditions, &mr, NULL, NULL, 1000) == 0);
        }

        /* Issue #122 (TA follow-up): cancellation must be preserved across the
         * refactor to pentax_wait_for_idle().  A cancel during BUSY/UNKNOWN polling
         * must exit promptly (return -1) without consuming the remaining bound, and
         * be distinct from "not idle" (which returns 0). */
        {
                struct mock_reader {
                        unsigned char frame[532];
                        int fail_read;
                        int calls;
                } mr;
                struct mock_cancel {
                        int cancel_after_calls;   /* report cancel once calls reach this */
                        int calls;
                } mc;

                int
                mock_read_conditions (void *user_data, unsigned char **data,
                                      unsigned int *size)
                {
                        struct mock_reader *m = user_data;
                        m->calls++;
                        *data = malloc (sizeof (m->frame));
                        if (*data)
                                memcpy (*data, m->frame, sizeof (m->frame));
                        *size = sizeof (m->frame);
                        return 0;                   /* PTP_RC_OK */
                }

                int
                mock_cancel (void *user_data)
                {
                        struct mock_cancel *m = user_data;
                        m->calls++;
                        return m->calls >= m->cancel_after_calls;
                }

                /* Cancel on the 2nd poll while the camera is BUSY: the wait must
                 * return -1 promptly, well before the large bound would exhaust.
                 * The cancel probe runs at the top of each iteration, so with
                 * cancel_after_calls=2 it fires before the 2nd read -> only 1 read. */
                memset (&mr, 0, sizeof (mr));
                put_u32le (mr.frame, 104, PENTAX_CONDITION_ACTIVITY_SHOOTING); /* BUSY */
                memset (&mc, 0, sizeof (mc));
                mc.cancel_after_calls = 2;
                CHECK (pentax_wait_for_idle (mock_read_conditions, &mr,
                                             mock_cancel, &mc, 60 * 1000) == -1);
                CHECK (mr.calls == 1);            /* exited promptly, not the full 60 s bound */

                /* No cancel callback: same BUSY camera still exhausts the (small) bound
                 * and reports "not idle" (0), proving cancellation is distinct from it. */
                memset (&mr, 0, sizeof (mr));
                put_u32le (mr.frame, 104, PENTAX_CONDITION_ACTIVITY_SHOOTING);
                CHECK (pentax_wait_for_idle (mock_read_conditions, &mr, NULL, NULL, 1000) == 0);

                /* Cancel on the very first poll: exits before any conditions read. */
                memset (&mr, 0, sizeof (mr));
                memset (&mc, 0, sizeof (mc));
                mc.cancel_after_calls = 1;
                CHECK (pentax_wait_for_idle (mock_read_conditions, &mr,
                                             mock_cancel, &mc, 60 * 1000) == -1);
                CHECK (mr.calls == 0);            /* cancelled before the first read */
        }

/* Astro-mode detection (IT2 parity): the live signal is the exposure-mode
	 * dial value == ExpMode.AstroTracer (20) or an in-progress astro shift.  The
	 * offset-504 capability bit alone must NOT count as "in astro mode". */
	{
		PentaxConditions g;
		memset (&g, 0, sizeof (g));
		CHECK (!pentax_conditions_in_astro_mode (&g));
		CHECK (!pentax_conditions_in_astro_mode (NULL));
		g.capability_flags = PENTAX_CONDITION_ASTROTRACER3; /* can do astro */
		CHECK (!pentax_conditions_in_astro_mode (&g));
		memset (&g, 0, sizeof (g));
		g.exposure_mode = PENTAX_EXP_MODE_ASTROTRACER;
		CHECK (pentax_conditions_in_astro_mode (&g));
		memset (&g, 0, sizeof (g));
		g.astro_status_flags = PENTAX_CONDITION_ASTRO_SHIFT_MODE;
		CHECK (pentax_conditions_in_astro_mode (&g));
	}

	/* Idle-wait gate (issue #122): ordinary single-shot captures skip the
	 * post-capture wait; multi-shot / astro / bulb keep it. */
	{
		PentaxConditions g;
		memset (&g, 0, sizeof (g));
		CHECK (!pentax_capture_needs_idle_wait (&g));
		CHECK (!pentax_capture_needs_idle_wait (NULL));
		g.activity_flags = PENTAX_CONDITION_ACTIVITY_MULTI_MODE;
		CHECK (pentax_capture_needs_idle_wait (&g));
		memset (&g, 0, sizeof (g));
		g.activity_flags = PENTAX_CONDITION_ACTIVITY_MULTI_CAPTURE;
		CHECK (pentax_capture_needs_idle_wait (&g));
		memset (&g, 0, sizeof (g));
		g.astro_status_flags = PENTAX_CONDITION_ASTRO_SHIFT_MODE;
		CHECK (pentax_capture_needs_idle_wait (&g));
		memset (&g, 0, sizeof (g));
		g.exposure_mode = PENTAX_EXP_MODE_ASTROTRACER; /* live astro mode */
		CHECK (pentax_capture_needs_idle_wait (&g));
		memset (&g, 0, sizeof (g));
		g.capability_flags = PENTAX_CONDITION_ASTROTRACER3; /* can, not in */
		CHECK (!pentax_capture_needs_idle_wait (&g));
                memset (&g, 0, sizeof (g));
                g.bulb_timer_seconds = 120;
                CHECK (pentax_capture_needs_idle_wait (&g));
        }

	/* A killed Benro appliance process leaves the K-3 III session resident.
	 * Reconnect reports 0x02fa, not SessionAlreadyOpened.  It must select the
	 * ordered close+port-reset recovery twice, then fail closed rather than
	 * entering a reset storm. */
	CHECK (pentax_session_error_needs_close_reset (0x02fa, 1));
	CHECK (pentax_session_error_needs_close_reset (0x02fd, 2));
	CHECK (pentax_session_error_needs_close_reset (0x02ff, 1));
	CHECK (!pentax_session_error_needs_close_reset (0x02fa, 3));
	CHECK (!pentax_session_error_needs_close_reset (0x201e, 1));

	free (buffer.data);
	return 0;
}
