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

uint32_t pentax_get_u32le (const unsigned char *data);
int pentax_capture_buffer_write (PentaxCaptureBuffer *buffer,
	const unsigned char *data, size_t size);
int pentax_capture_buffer_seek (PentaxCaptureBuffer *buffer,
	unsigned int operation, int32_t displacement);
int pentax_candidate_filename (const unsigned char *data, uint32_t size,
	char *filename, size_t filename_size);

#endif
