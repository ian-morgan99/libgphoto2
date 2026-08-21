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
	unsigned char condition_data[532] = {0};
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
	MockTransfer mock;
	PentaxTransferOps transfer_operations;

	CHECK (pentax_get_u32le ((const unsigned char *)"\x78\x56\x34\x12") ==
		0x12345678U);
	CHECK (pentax_lookup_model (0x25fb, 0x0189, "PENTAX K-3 Mark III",
		&model_no, &extension_version));
	CHECK ((model_no == 78420) && (extension_version == 1));
	CHECK (pentax_lookup_model (0x25fb, 0x0183, "PENTAX K-1 Mark II",
		&model_no, &extension_version));
	CHECK ((model_no == 78400) && (extension_version == 1));
	CHECK (pentax_model_uses_new_focus (PENTAX_MODEL_K3_MARK_III));
	CHECK (!pentax_model_uses_new_focus (PENTAX_MODEL_K1_MARK_II));
	CHECK (!pentax_model_uses_new_focus (0));
	CHECK (!pentax_lookup_model (0x25fb, 0x018f,
		"PENTAX K-3 Mark III Monochrome", &model_no, &extension_version));
	CHECK ((model_no == 0) && (extension_version == 0));
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
		(conditions.bulb_timer_denominator == 1) && (conditions.iso == 3200) &&
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
	CHECK (pentax_transfer_run (NULL, &transfer_operations) ==
		GP_ERROR_BAD_PARAMETERS);
	CHECK (pentax_minimum_focus_displacement (35, 1, &displacement) == GP_OK);
	CHECK (displacement == 29);
	CHECK (pentax_minimum_focus_displacement (35, -1, &displacement) == GP_OK);
	CHECK (displacement == -29);
	CHECK (pentax_minimum_focus_displacement (0, 1, &displacement) == GP_ERROR_CORRUPTED_DATA);
	CHECK (pentax_minimum_focus_displacement (35, 0, &displacement) == GP_ERROR_BAD_PARAMETERS);

	free (buffer.data);
	return 0;
}
