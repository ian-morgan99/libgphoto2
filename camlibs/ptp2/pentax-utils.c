#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gphoto2/gphoto2-result.h>

#include "pentax-utils.h"

#define PENTAX_CAPTURE_MAX_FILE_SIZE ((size_t)2U * 1024U * 1024U * 1024U)

uint32_t
pentax_get_u32le (const unsigned char *data)
{
	return ((uint32_t)data[0]) |
	       ((uint32_t)data[1] << 8) |
	       ((uint32_t)data[2] << 16) |
	       ((uint32_t)data[3] << 24);
}

int
pentax_lookup_model (uint16_t usb_vendor, uint16_t usb_product,
		const char *device_model, uint32_t *model_no, uint32_t *extension_version)
{
	if (!model_no || !extension_version)
		return 0;
	*model_no = 0;
	*extension_version = 0;
	if (!device_model || (usb_vendor != 0x25fb))
		return 0;
	if ((usb_product == 0x018c) &&
	    !strcmp (device_model, "PENTAX K-3 Mark III")) {
		*model_no = 78420;
		*extension_version = 1;
		return 1;
	}
	if ((usb_product == 0x0183) &&
	    !strcmp (device_model, "PENTAX K-1 Mark II")) {
		*model_no = 78400;
		*extension_version = 1;
		return 1;
	}
	return 0;
}

static int
pentax_capture_buffer_reserve (PentaxCaptureBuffer *buffer, size_t required)
{
	size_t capacity;
	unsigned char *data;

	if (required > PENTAX_CAPTURE_MAX_FILE_SIZE)
		return GP_ERROR_FIXED_LIMIT_EXCEEDED;
	if (required <= buffer->capacity)
		return GP_OK;
	capacity = buffer->capacity ? buffer->capacity : 1024U * 1024U;
	while (capacity < required) {
		if (capacity > PENTAX_CAPTURE_MAX_FILE_SIZE / 2) {
			capacity = PENTAX_CAPTURE_MAX_FILE_SIZE;
			break;
		}
		capacity *= 2;
	}
	data = realloc (buffer->data, capacity);
	if (!data)
		return GP_ERROR_NO_MEMORY;
	buffer->data = data;
	buffer->capacity = capacity;
	return GP_OK;
}

int
pentax_capture_buffer_write (PentaxCaptureBuffer *buffer,
		const unsigned char *data, size_t size)
{
	size_t end;
	int ret;

	if (!buffer || (!data && size))
		return GP_ERROR_BAD_PARAMETERS;
	if (size > PENTAX_CAPTURE_MAX_FILE_SIZE - buffer->offset)
		return GP_ERROR_FIXED_LIMIT_EXCEEDED;
	end = buffer->offset + size;
	ret = pentax_capture_buffer_reserve (buffer, end);
	if (ret < GP_OK)
		return ret;
	if (buffer->offset > buffer->size)
		memset (buffer->data + buffer->size, 0, buffer->offset - buffer->size);
	if (size)
		memcpy (buffer->data + buffer->offset, data, size);
	buffer->offset = end;
	if (end > buffer->size)
		buffer->size = end;
	return GP_OK;
}

int
pentax_capture_buffer_seek (PentaxCaptureBuffer *buffer, unsigned int operation,
		int32_t displacement)
{
	int64_t base, destination;

	if (!buffer)
		return GP_ERROR_BAD_PARAMETERS;
	switch (operation) {
	case 4: base = 0; break;
	case 5: base = (int64_t)buffer->offset; break;
	case 6: base = (int64_t)buffer->size; break;
	default: return GP_ERROR_BAD_PARAMETERS;
	}
	destination = base + displacement;
	if ((destination < 0) || ((uint64_t)destination > PENTAX_CAPTURE_MAX_FILE_SIZE))
		return GP_ERROR_BAD_PARAMETERS;
	buffer->offset = (size_t)destination;
	return GP_OK;
}

int
pentax_candidate_filename (const unsigned char *data, uint32_t size,
		char *filename, size_t filename_size)
{
	size_t characters, i, output = 0;

	if (!data || (size < 4) || !filename || (filename_size < 2))
		return GP_ERROR_BAD_PARAMETERS;
	characters = data[3];
	if ((characters > (size - 4) / 2) || !characters)
		return GP_ERROR_CORRUPTED_DATA;
	for (i = 0; i < characters; i++) {
		uint16_t character = data[4 + i * 2] | ((uint16_t)data[5 + i * 2] << 8);
		if (!character)
			break;
		if ((character < 0x20) || (character > 0x7e) ||
		    (character == '/') || (character == '\\'))
			return GP_ERROR_CORRUPTED_DATA;
		if (output + 1 >= filename_size)
			return GP_ERROR_FIXED_LIMIT_EXCEEDED;
		filename[output++] = (char)character;
	}
	if (!output)
		return GP_ERROR_CORRUPTED_DATA;
	filename[output] = '\0';
	if (!strcmp (filename, ".") || !strcmp (filename, ".."))
		return GP_ERROR_CORRUPTED_DATA;
	return GP_OK;
}

int
pentax_jpeg_bounds (const unsigned char *data, size_t size,
		size_t *offset, size_t *length)
{
	size_t start, end;

	if (!data || !offset || !length)
		return GP_ERROR_BAD_PARAMETERS;
	*offset = 0;
	*length = 0;
	for (start = 0; start + 1 < size; start++)
		if ((data[start] == 0xff) && (data[start + 1] == 0xd8))
			break;
	if (start + 1 >= size)
		return GP_ERROR_CORRUPTED_DATA;
	for (end = start + 2; end + 1 < size; end++)
		if ((data[end] == 0xff) && (data[end + 1] == 0xd9)) {
			*offset = start;
			*length = end + 2 - start;
			return GP_OK;
		}
	return GP_ERROR_CORRUPTED_DATA;
}

static int
pentax_transfer_interrupted (const PentaxTransferOps *operations)
{
	if (operations->is_cancelled &&
	    operations->is_cancelled (operations->user_data))
		return GP_ERROR_CANCEL;
	if (operations->is_timed_out &&
	    operations->is_timed_out (operations->user_data))
		return GP_ERROR_TIMEOUT;
	return GP_OK;
}

int
pentax_transfer_run (PentaxCaptureBuffer *buffer,
		const PentaxTransferOps *operations)
{
	unsigned int command_count = 0;
	int ret;

	if (!buffer || !operations || !operations->get_command ||
	    !operations->get_block || !operations->max_block_size)
		return GP_ERROR_BAD_PARAMETERS;
	for (;;) {
		uint8_t operation = 0;
		int32_t operation_info = 0;

		if (++command_count > 100000U)
			return GP_ERROR_FIXED_LIMIT_EXCEEDED;
		ret = pentax_transfer_interrupted (operations);
		if (ret < GP_OK)
			return ret;
		ret = operations->get_command (operations->user_data, &operation,
			&operation_info);
		if (ret < GP_OK)
			return ret;
		if (command_count == 1) {
			if (operation != 1)
				return GP_ERROR_CORRUPTED_DATA;
			continue;
		}
		if (operation == 1)
			return GP_ERROR_CORRUPTED_DATA;
		if (operation == 2)
			return buffer->size ? GP_OK : GP_ERROR_CORRUPTED_DATA;
		if (operation == 3) {
			uint32_t remaining;

			if (operation_info <= 0)
				return GP_ERROR_CORRUPTED_DATA;
			remaining = (uint32_t)operation_info;
			while (remaining) {
				unsigned char *data = NULL;
				uint32_t request = remaining;
				uint32_t transferred = 0;

				ret = pentax_transfer_interrupted (operations);
				if (ret < GP_OK)
					return ret;
				if (request > operations->max_block_size)
					request = operations->max_block_size;
				ret = operations->get_block (operations->user_data, request,
					&data, &transferred);
				if (ret < GP_OK) {
					free (data);
					return ret;
				}
				if (!data || !transferred || (transferred > request)) {
					free (data);
					return GP_ERROR_CORRUPTED_DATA;
				}
				ret = pentax_capture_buffer_write (buffer, data, transferred);
				free (data);
				if (ret < GP_OK)
					return ret;
				remaining -= transferred;
				if (transferred < operations->max_block_size)
					break;
			}
			continue;
		}
		if ((operation >= 4) && (operation <= 6)) {
			ret = pentax_capture_buffer_seek (buffer, operation, operation_info);
			if (ret < GP_OK)
				return ret;
			continue;
		}
		return GP_ERROR_NOT_SUPPORTED;
	}
}
