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

int
main (void)
{
	PentaxCaptureBuffer buffer = {0};
	const unsigned char filename[] = {
		1, 2, 3, 11,
		'I', 0, 'M', 0, 'G', 0, '0', 0, '0', 0, '0', 0, '1', 0, '.', 0,
		'J', 0, 'P', 0, 'G', 0
	};
	unsigned char malformed[sizeof (filename)];
	const unsigned char first[] = {1, 2, 3, 4};
	const unsigned char patch[] = {9, 8};
	char name[32];
	uint32_t model_no = 99, extension_version = 99;

	CHECK (pentax_get_u32le ((const unsigned char *)"\x78\x56\x34\x12") ==
		0x12345678U);
	CHECK (pentax_lookup_model (0x25fb, 0x018c, "PENTAX K-3 Mark III",
		&model_no, &extension_version));
	CHECK ((model_no == 78420) && (extension_version == 1));
	CHECK (pentax_lookup_model (0x25fb, 0x0183, "PENTAX K-1 Mark II",
		&model_no, &extension_version));
	CHECK ((model_no == 78400) && (extension_version == 1));
	CHECK (!pentax_lookup_model (0x25fb, 0x018f,
		"PENTAX K-3 Mark III Monochrome", &model_no, &extension_version));
	CHECK ((model_no == 0) && (extension_version == 0));
	CHECK (!pentax_lookup_model (0x25fb, 0x018c,
		"PENTAX K-3 Mark III v1.9", &model_no, &extension_version));
	CHECK (!pentax_lookup_model (0x1234, 0x018c, "PENTAX K-3 Mark III",
		&model_no, &extension_version));
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

	free (buffer.data);
	return 0;
}
