#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gphoto2/gphoto2-result.h>

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

	CHECK (pentax_get_u32le ((const unsigned char *)"\x78\x56\x34\x12") ==
		0x12345678U);

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
	CHECK (pentax_model_uses_new_focus (PENTAX_MODEL_K3_MARK_III));
	CHECK (!pentax_model_uses_new_focus (PENTAX_MODEL_K1_MARK_II));
	CHECK (!pentax_model_uses_new_focus (0));
	/* The Monochrome shares the K-3 III protocol family; IT2 matches it
	 * with StartsWith("PENTAX K-3 Mark III") so it resolves to the same
	 * model_no 78420 and all K-3 III capability flags. */
	CHECK (pentax_lookup_model (0x25fb, 0x018f,
		"PENTAX K-3 Mark III Monochrome", &model_no, &extension_version));
	CHECK ((model_no == PENTAX_MODEL_K3_MARK_III) &&
		(extension_version == 1));
	CHECK (!pentax_model_uses_new_focus (PENTAX_MODEL_K3_MARK_III_MONO) ==
		!!pentax_model_uses_new_focus (PENTAX_MODEL_K3_MARK_III));
	CHECK (!pentax_lookup_model (0x25fb, 0x0189,
		"PENTAX K-3 Mark III v1.9", &model_no, &extension_version));
	CHECK (!pentax_lookup_model (0x1234, 0x0189, "PENTAX K-3 Mark III",
		&model_no, &extension_version));
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
	CHECK (pentax_model_supports_card_writing_mode (PENTAX_MODEL_K3_MARK_III_MONO));
	CHECK (pentax_model_supports_card_writing_mode (PENTAX_MODEL_645D));
	CHECK (pentax_model_supports_card_writing_mode (PENTAX_MODEL_645Z));
	CHECK (!pentax_model_supports_card_writing_mode (PENTAX_MODEL_KP));
	CHECK (!pentax_model_supports_card_writing_mode (PENTAX_MODEL_K70));
	CHECK (!pentax_model_supports_card_writing_mode (PENTAX_MODEL_GR_III));
	CHECK (!pentax_model_supports_card_writing_mode (PENTAX_MODEL_KF));

	/* Cross process (d02c): K-1 II probe (2026-09-02) shows GET/SET
	 * unsupported even in cross-process mode, so gate on k3iii family. */
	CHECK (pentax_model_supports_cross_process (PENTAX_MODEL_K3_MARK_III));
	CHECK (pentax_model_supports_cross_process (PENTAX_MODEL_K3_MARK_III_MONO));
	CHECK (!pentax_model_supports_cross_process (PENTAX_MODEL_K1_MARK_II));
	CHECK (!pentax_model_supports_cross_process (PENTAX_MODEL_K1));
	CHECK (!pentax_model_supports_cross_process (PENTAX_MODEL_K3));
	CHECK (!pentax_model_supports_cross_process (PENTAX_MODEL_KP));

        /* Star AF / AF Active Area (0xd038, issue #76): K-3 III family only.
         * The model gate is necessary but not sufficient — the descriptor must
         * also advertise the enum at runtime for activated bodies. */
        CHECK (pentax_model_supports_star_af (PENTAX_MODEL_K3_MARK_III));
        CHECK (pentax_model_supports_star_af (PENTAX_MODEL_K3_MARK_III_MONO));

	/* Research-capable PIDs (issue #19): only the three vendor bodies whose
	 * capture flow we exercise may enter research paths. K-3 II (0x017b) is
	 * deliberately absent from IT2, so it must fail closed here too. */
	CHECK (pentax_pid_is_research_capable (0x0183)); /* K-1 II */
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

	conditions.bulb_timer_seconds = 120; /* 121s + margin beats the base */
	CHECK (pentax_capture_timeout_ms (&conditions) == 151000);

	memset (&conditions, 0, sizeof (conditions));
	conditions.activity_flags = PENTAX_CONDITION_ACTIVITY_MULTI_MODE;
	CHECK (pentax_capture_timeout_ms (&conditions) == 270000); /* 4x + margin */

	conditions.bulb_timer_seconds = 9; /* bulb + multi-shot composite */
	CHECK (pentax_capture_timeout_ms (&conditions) == 70000);

	memset (&conditions, 0, sizeof (conditions));
	conditions.astro_status_flags = PENTAX_CONDITION_ASTRO_SHIFT_MODE;
	conditions.has_astro_limit = 1;
	conditions.astro_limit_seconds = 300; /* 301s + margin */
	CHECK (pentax_capture_timeout_ms (&conditions) == 331000);

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
	conditions.astro_limit_seconds = 300; /* 301s + margin */
	CHECK (pentax_capture_timeout_ms (&conditions) == 331000);

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
	CHECK (pentax_recovery_probe_ok (condition_data, sizeof (condition_data)));

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
	 * timer, denominator 1) must render as integer seconds, not as the
	 * fraction "1/<n>" that made a 2-minute Bulb read like a minute. */
	{
		char fmt[64];
		CHECK (pentax_format_shutter_speed (0, fmt, sizeof (fmt)) == 0);
		CHECK (!strcmp (fmt, "Auto"));
		/* 120 s Bulb timer: low 32 = 120, high 32 = 1. */
		CHECK (pentax_format_shutter_speed (((uint64_t)1 << 32) | 120ULL, fmt, sizeof (fmt)) == 0);
		CHECK (!strcmp (fmt, "120s"));
		/* 30 s Bulb timer. */
		CHECK (pentax_format_shutter_speed (((uint64_t)1 << 32) | 30ULL, fmt, sizeof (fmt)) == 0);
		CHECK (!strcmp (fmt, "30s"));
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

	free (buffer.data);
	return 0;
}
