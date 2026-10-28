#include "config.h"

#include <stdio.h>
#include <string.h>

#include "pslr_utils.h"
#include "pslr_model.h"

#define CHECK_TEST(expression) do { \
	if (!(expression)) { \
		fprintf (stderr, "FAIL line %d: %s\n", __LINE__, #expression); \
		return 1; \
	} \
} while (0)

int
main (void)
{
	pslr_rational_t value;
	ipslr_model_info_t *k5ii;

	/* The official local K-5 II v1.07 package identifies the body as the
	 * legacy USB-SCSI model represented by 0x12f70 in the parser table. Keep
	 * that lookup and its byte-order/status-parser contract covered without
	 * treating a firmware header as hardware qualification. */
	k5ii = pslr_find_model_by_id (0x12f70);
	CHECK_TEST (k5ii != NULL);
	CHECK_TEST (!strcmp (k5ii->name, "K-5II"));
	CHECK_TEST (!k5ii->is_little_endian);
	CHECK_TEST (k5ii->status_buffer_size == 444);
	CHECK_TEST (k5ii->max_jpeg_stars == 4);
	CHECK_TEST (k5ii->jpeg_resolutions[0] == 16);
	CHECK_TEST (k5ii->jpeg_resolutions[3] == 2);
	CHECK_TEST (k5ii->status_parser_function != NULL);
	CHECK_TEST (pslr_find_model_by_id (0x25fb0148) == NULL);

	value = parse_aperture ("2.8");
	CHECK_TEST (value.nom == 28 && value.denom == 10);
	value = parse_aperture ("16");
	CHECK_TEST (value.nom == 16 && value.denom == 1);
	CHECK_TEST (parse_aperture ("2.8x").nom == 0);
	CHECK_TEST (parse_aperture ("nan").nom == 0);
	CHECK_TEST (parse_aperture ("101").nom == 0);

	value = parse_shutter_speed ("1/125");
	CHECK_TEST (value.nom == 1 && value.denom == 125);
	value = parse_shutter_speed ("5s");
	CHECK_TEST (value.nom == 5 && value.denom == 1);
	value = parse_shutter_speed ("5");
	CHECK_TEST (value.nom == 5 && value.denom == 1);
	value = parse_shutter_speed ("0.5");
	CHECK_TEST (value.nom == 500 && value.denom == 1000);
	CHECK_TEST (parse_shutter_speed ("1/0").nom == 0);
	CHECK_TEST (parse_shutter_speed ("1/125x").nom == 0);
	CHECK_TEST (parse_shutter_speed ("nan").nom == 0);

	return 0;
}
