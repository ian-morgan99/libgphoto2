#include <stdint.h>
#include <stdio.h>

int
translate_ptp_result (uint16_t result)
{
	fprintf (stderr, "probe PTP translation fallback: 0x%04x\n", result);
	return -100 - (int)result;
}
