#include "config.h"

#include <stdio.h>

#include "../camlibs/ptp2/ptp.h"

/* The object-parent root convention is generic PTP2 behavior.  Keep the MTP
 * 0xffffffff sentinel test separate from Pentax tests: a Pentax capture may
 * expose the same folder reconstruction path, but the rule belongs to the
 * object model, not a camera vendor. */
int
main (void)
{
	if (!ptp_object_parent_is_root (PTP_HANDLER_ROOT) ||
	    !ptp_object_parent_is_root (PTP_HANDLER_SPECIAL) ||
	    ptp_object_parent_is_root (0x00000001U)) {
		fprintf (stderr, "PTP object-parent root classification failed\n");
		return 1;
	}
	return 0;
}
