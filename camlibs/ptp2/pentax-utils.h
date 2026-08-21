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

uint32_t pentax_get_u32le (const unsigned char *data);
int pentax_lookup_model (uint16_t usb_vendor, uint16_t usb_product,
	const char *device_model, uint32_t *model_no, uint32_t *extension_version);
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
