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

static uint16_t
pentax_get_u16le (const unsigned char *data)
{
	return (uint16_t)(data[0] | ((uint16_t)data[1] << 8));
}

int
pentax_parse_live_view_geometry (const unsigned char *data, size_t size,
		PentaxLiveViewGeometry *geometry)
{
	PentaxLiveViewGeometry parsed;

	if (!data || !geometry)
		return GP_ERROR_BAD_PARAMETERS;
	if (size < 20)
		return GP_ERROR_CORRUPTED_DATA;
	parsed.area_width = pentax_get_u16le (data + 4);
	parsed.area_height = pentax_get_u16le (data + 6);
	parsed.active_width = pentax_get_u16le (data + 8);
	parsed.active_height = pentax_get_u16le (data + 10);
	parsed.contrast_af_active_width = pentax_get_u16le (data + 12);
	parsed.contrast_af_active_height = pentax_get_u16le (data + 14);
	parsed.contrast_af_spot_width = pentax_get_u16le (data + 16);
	parsed.contrast_af_spot_height = pentax_get_u16le (data + 18);
	if (!parsed.area_width || !parsed.area_height ||
	    (parsed.active_width > parsed.area_width) ||
	    (parsed.active_height > parsed.area_height) ||
	    (parsed.contrast_af_active_width > parsed.area_width) ||
	    (parsed.contrast_af_active_height > parsed.area_height) ||
	    (parsed.contrast_af_spot_width > parsed.contrast_af_active_width) ||
	    (parsed.contrast_af_spot_height > parsed.contrast_af_active_height))
		return GP_ERROR_CORRUPTED_DATA;
	*geometry = parsed;
	return GP_OK;
}

int
pentax_parse_live_view_af_position (const unsigned char *data, size_t size,
		const PentaxLiveViewGeometry *geometry, uint16_t *x, uint16_t *y)
{
	uint16_t parsed_x, parsed_y;

	if (!data || !geometry || !x || !y)
		return GP_ERROR_BAD_PARAMETERS;
	if (size == 4) {
		parsed_x = geometry->area_width / 2;
		parsed_y = geometry->area_height / 2;
	} else if (size >= 8) {
		parsed_x = pentax_get_u16le (data + 4);
		parsed_y = pentax_get_u16le (data + 6);
	} else {
		return GP_ERROR_CORRUPTED_DATA;
	}
	if ((parsed_x >= geometry->area_width) ||
	    (parsed_y >= geometry->area_height))
		return GP_ERROR_CORRUPTED_DATA;
	*x = parsed_x;
	*y = parsed_y;
	return GP_OK;
}

int
pentax_encode_live_view_af_position (uint16_t x, uint16_t y,
		unsigned char data[8])
{
	if (!data)
		return GP_ERROR_BAD_PARAMETERS;
	memset (data, 0, 8);
	data[0] = 2;
	data[4] = (unsigned char)x;
	data[5] = (unsigned char)(x >> 8);
	data[6] = (unsigned char)y;
	data[7] = (unsigned char)(y >> 8);
	return GP_OK;
}

int
pentax_encode_live_view_zoom (uint16_t x, uint16_t y,
		uint8_t magnification, unsigned char data[12])
{
	if (!data || !magnification)
		return GP_ERROR_BAD_PARAMETERS;
	memset (data, 0, 12);
	data[0] = 4;
	data[4] = (unsigned char)x;
	data[5] = (unsigned char)(x >> 8);
	data[6] = (unsigned char)y;
	data[7] = (unsigned char)(y >> 8);
	data[8] = magnification;
	return GP_OK;
}

int
pentax_live_view_stop_response_ok (uint16_t response)
{
	return (response == 0x2001) || (response == 0xa005);
}

int
pentax_live_view_zoom_fallback (uint8_t requested, uint16_t response,
		uint8_t *fallback)
{
	if (!fallback)
		return GP_ERROR_BAD_PARAMETERS;
	if ((requested != 16) || (response != 0x201c))
		return 0;
	*fallback = 10;
	return 1;
}

int
pentax_live_view_frame_should_retry (uint16_t response,
		unsigned int attempts, unsigned int elapsed_ms)
{
	/* IT2 identifies 0xa008 as NoUpdateImage.  Thirty attempts at its 33 ms
	 * cadence are permitted, with an independent 1.5 second wall-time cap. */
	return (response == 0xa008) && (attempts < 30) && (elapsed_ms < 1500);
}

int
pentax_parse_conditions (const unsigned char *data, size_t size,
		PentaxConditions *conditions)
{
	PentaxConditions parsed;

	if (!data || !conditions)
		return GP_ERROR_BAD_PARAMETERS;
	/* capability_flags at 504 is the final mandatory field. */
	if (size < 508)
		return GP_ERROR_CORRUPTED_DATA;
	memset (&parsed, 0, sizeof (parsed));
	parsed.operation_state = (uint8_t)pentax_get_u32le (data + 24);
	parsed.activity_flags = pentax_get_u32le (data + 104);
	parsed.exposure_step = pentax_get_u32le (data + 168);
	parsed.exposure_mode = pentax_get_u32le (data + 184);
	parsed.user_mode = pentax_get_u32le (data + 40);
	parsed.bulb_timer_seconds = pentax_get_u32le (data + 272);
	parsed.bulb_timer_denominator = pentax_get_u32le (data + 276);
	parsed.aperture_numerator = pentax_get_u32le (data + 280);
	parsed.aperture_denominator = pentax_get_u32le (data + 284);
	parsed.exposure_comp_numerator = (int32_t)pentax_get_u32le (data + 288);
	parsed.exposure_comp_denominator = pentax_get_u32le (data + 292);
	parsed.iso = pentax_get_u32le (data + 312);
	parsed.astro_status_flags = pentax_get_u32le (data + 320);
	parsed.open_av_num = pentax_get_u32le (data + 328);
	parsed.drive_mode = pentax_get_u32le (data + 492);
	/* IT2 offset 196: AF mode; 0 = MF, >0 = an AF mode. */
	parsed.af_mode = pentax_get_u32le (data + 196);
	parsed.capability_flags = pentax_get_u32le (data + 504);
	if (size >= 532) {
		parsed.astro_limit_seconds = pentax_get_u32le (data + 528);
		parsed.has_astro_limit = 1;
	}
	*conditions = parsed;
	return GP_OK;
}

int
pentax_minimum_focus_displacement (uint32_t open_av_num, int direction,
		int32_t *displacement)
{
	uint64_t magnitude;

	if (!displacement || ((direction != -1) && (direction != 1)))
		return GP_ERROR_BAD_PARAMETERS;
	/* Image Transmitter 2 uses (int)(openAvNum * 2.5 / 3.0). */
	magnitude = ((uint64_t)open_av_num * 5U) / 6U;
	if (!magnitude || (magnitude > INT32_MAX))
		return GP_ERROR_CORRUPTED_DATA;
	*displacement = direction > 0 ? (int32_t)magnitude : -(int32_t)magnitude;
	return GP_OK;
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
	if ((usb_product == 0x0189) &&
	    !strcmp (device_model, "PENTAX K-3 Mark III")) {
		*model_no = PENTAX_MODEL_K3_MARK_III;
		*extension_version = 1;
		return 1;
	}
	/* IT2 matches the Monochrome with StartsWith("PENTAX K-3 Mark III"),
	 * so it shares model_no 78420 and all K-3 III capability flags. */
	if ((usb_product == 0x018f) &&
	    !strncmp (device_model, "PENTAX K-3 Mark III", strlen ("PENTAX K-3 Mark III"))) {
		*model_no = PENTAX_MODEL_K3_MARK_III;
		*extension_version = 1;
		return 1;
	}
	if ((usb_product == 0x017f) &&
	    !strncmp (device_model, "PENTAX KP", strlen ("PENTAX KP"))) {
		*model_no = PENTAX_MODEL_KP;
		*extension_version = 1;
		return 1;
	}
	if ((usb_product == 0x017d) &&
	    !strncmp (device_model, "PENTAX K-70", strlen ("PENTAX K-70"))) {
		*model_no = PENTAX_MODEL_K70;
		*extension_version = 1;
		return 1;
	}
	if ((usb_product == 0x0183) &&
	    !strcmp (device_model, "PENTAX K-1 Mark II")) {
		*model_no = PENTAX_MODEL_K1_MARK_II;
		*extension_version = 1;
		return 1;
	}
	/* IT2 Model setter: 645D uses StartsWith("645D"), model 77320, and is
	 * the only IT2 model with vendor extension version 0.  IT2's IsSupported
	 * matches the bare string "645D" (its manufacturer field is "PENTAX",
	 * not "RICOH IMAGING COMPANY, LTD."), so accept both forms. */
	if ((usb_product == 0x0130) &&
	    (!strcmp (device_model, "645D") ||
	     !strncmp (device_model, "PENTAX 645D", strlen ("PENTAX 645D")))) {
		*model_no = PENTAX_MODEL_645D;
		*extension_version = 0;
		return 1;
	}
	/* K-3 II is NOT in IT2, so we have no normative reference for it.
	 * Fail-closed: assume the older K-3-generation architecture (old
	 * transfer, old focus 0x9016) until proven otherwise on hardware or
	 * via a newer IT2 build.  Not listed here, so vendor mode stays off. */
	/* IT2 Model setter: K-3 (77760), K-1 (77970), GR III (78350) all use
	 * vendor extension version 1. */
	if ((usb_product == 0x0165) &&
	    !strncmp (device_model, "PENTAX K-3", strlen ("PENTAX K-3"))) {
		*model_no = PENTAX_MODEL_K3;
		*extension_version = 1;
		return 1;
	}
	if ((usb_product == 0x0179) &&
	    !strncmp (device_model, "PENTAX K-1", strlen ("PENTAX K-1"))) {
		*model_no = PENTAX_MODEL_K1;
		*extension_version = 1;
		return 1;
	}
	if ((usb_product == 0x210f) &&
	    !strncmp (device_model, "RICOH GR III", strlen ("RICOH GR III"))) {
		*model_no = PENTAX_MODEL_GR_III;
		*extension_version = 1;
		return 1;
	}
	return 0;
}

int
pentax_model_uses_new_focus (uint32_t model_no)
{
	/* IT2 explicitly selects 0x9017 only for its new-focus models.  The
	 * K-1 II is an old-focus 0x9016 model and must fail closed here.
	 * Per IT2 Model setter: new-focus = K-3 III family, KP, GR III. */
	return (model_no == PENTAX_MODEL_K3_MARK_III) ||
	       (model_no == PENTAX_MODEL_KP) ||
	       (model_no == PENTAX_MODEL_GR_III);
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
