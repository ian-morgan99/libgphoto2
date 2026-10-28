#include "config.h"

#include <stdio.h>

#include "pslr_utils.h"

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
