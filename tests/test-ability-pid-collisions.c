/**
 * Regression test: two ability rows must not claim the same (vendor, product)
 * on port types with different reachability from a plain USB probe unless
 * the shared identity is explicitly accounted for below.
 *
 * WHY THIS EXISTS (issue benro-polaris-firmware-patcher #379)
 * -----------------------------------------------------------
 * camlibs/pentax/library.c sets a.port = GP_PORT_USB_SCSI once and reuses that
 * struct for every appended row, so all Pentax SCSI abilities are only ever
 * considered when the probed port type is GP_PORT_USB_SCSI. camlibs/ptp2 rows
 * are GP_PORT_USB. Those bits are disjoint (1<<6 vs 1<<2), so during USB
 * autodetection gp_abilities_list_detect_usb() skips every Pentax row:
 *
 *     if (!(list->abilities[i].port & port->type))
 *             continue;
 *
 * When ptp2 also carries a row for the same PID, that row wins by default and
 * PTP initialisation is then attempted against a body that exposes no PTP
 * interface. Observed on a K-01 in MSC mode (25fb:0130):
 *
 *     gp_camera_set_abilities: Setting abilities ('Pentax 645D (PTP Mode)')...
 *     *** Error ***  PTP I/O Error
 *     ----- gp_camera_init ret -7
 *
 * The misidentification and the -7 are one fault, not two.
 *
 * POLICY
 * ------
 * New collisions fail. The collisions already known are listed in KNOWN below;
 * if one of them is fixed, this test reports it as stale so the allowlist can
 * be tightened rather than silently keeping a dead entry.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gphoto2/gphoto2-abilities-list.h>
#include <gphoto2/gphoto2-port-info-list.h>

#define CHECK(x) do { \
	int __ec = (x); \
	if (__ec < 0) { \
		fprintf(stderr, "FAIL: %s:%d: %s returned %d\n", \
			__FILE__, __LINE__, #x, __ec); \
		exit(EXIT_FAILURE); \
	} \
} while (0)

/* Collisions that exist today and are deliberately left in place. Each needs a
 * physical body attached to change safely, because ability-table order affects
 * detection for every model in the table. */
struct known {
	int vendor;
	int product;
	const char *why;
};

static const struct known KNOWN[] = {
	{ 0x25fb, 0x0130,
	  "645D PTP and K-01 MSC share this ID; libusb1 excludes the "
	  "K-01 SCSI-only interface (patcher #379)" },
	{ 0x25fb, 0x0182,
	  "ptp2 'K-1 Mark II (MSC mode)' shadows pentax K-1 II MSC; fix gated on "
	  "device evidence (patcher #15/#379)" },
};

static int known_index(int v, int p)
{
	size_t i;
	for (i = 0; i < sizeof(KNOWN)/sizeof(KNOWN[0]); i++)
		if (KNOWN[i].vendor == v && KNOWN[i].product == p)
			return (int)i;
	return -1;
}

/* The hazard is asymmetric reachability, not shared port bits.
 *
 * A row is reachable by a plain USB autodetect probe iff its port mask has
 * GP_PORT_USB. When two rows share a PID and exactly one of them is reachable,
 * the probe can only ever select the reachable one -- the other driver becomes
 * unreachable for that body no matter what it supports. That is precisely the
 * K-01 case: pentax is GP_PORT_USB_SCSI (unreachable from a USB probe), ptp2 is
 * GP_PORT_USB, so ptp2 always wins and PTP init then fails against an MSC body.
 *
 * Two rows on identical port masks are ordinary duplicates handled by match
 * order; they are not this fault. */
static int shadowed(GPPortType a, GPPortType b)
{
	int a_usb = (a & GP_PORT_USB) != 0;
	int b_usb = (b & GP_PORT_USB) != 0;

	return a_usb != b_usb;
}

int main(void)
{
	CameraAbilitiesList *al = NULL;
	CameraAbilities reach, unreach;
	int count, i, j;
	int failures = 0;
	int stale = 0;
	int ptp_loaded = 0, pentax_loaded = 0;
	int matched[sizeof(KNOWN)/sizeof(KNOWN[0])];

	memset(matched, 0, sizeof(matched));
	CHECK (gp_abilities_list_new (&al));
	CHECK (gp_abilities_list_load (al, NULL));
	count = gp_abilities_list_count (al);
	if (count <= 0) {
		fprintf(stderr, "FAIL: no abilities loaded (%d)\n", count);
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < count; i++) {
		CameraAbilities a;
		CHECK (gp_abilities_list_get_abilities (al, i, &a));
		if (!strcmp(a.id, "PTP"))
			ptp_loaded = 1;
		if (!strcmp(a.id, "pentax"))
			pentax_loaded = 1;
		if (!a.usb_vendor || !a.usb_product)
			continue;

		for (j = i + 1; j < count; j++) {
			CameraAbilities b;
			int k;

			CHECK (gp_abilities_list_get_abilities (al, j, &b));
			if (b.usb_vendor != a.usb_vendor ||
			    b.usb_product != a.usb_product)
				continue;
			if (!shadowed(a.port, b.port))
				continue;

			k = known_index(a.usb_vendor, a.usb_product);
			if (k >= 0) {
				matched[k] = 1;
				continue;
			}

			failures++;
			/* Exactly one of the two is USB-reachable; name the loser. */
			if (a.port & GP_PORT_USB) {
				reach = a; unreach = b;
			} else {
				reach = b; unreach = a;
			}
			fprintf(stderr,
				"SHADOWED DRIVER: %04x:%04x\n"
				"    reachable   : %s (%s) port=0x%x\n"
				"    UNREACHABLE : %s (%s) port=0x%x\n"
				"  A plain USB probe cannot reach the second row; it "
				"will always select the first.\n",
				a.usb_vendor, a.usb_product,
				reach.model, reach.library, (unsigned)reach.port,
				unreach.model, unreach.library, (unsigned)unreach.port);
		}
	}

	for (i = 0; i < (int)(sizeof(KNOWN)/sizeof(KNOWN[0])); i++) {
		if (ptp_loaded && pentax_loaded && !matched[i]) {
			stale++;
			fprintf(stderr,
				"STALE ALLOWLIST ENTRY: %04x:%04x no longer "
				"collides -- remove it (%s)\n",
				KNOWN[i].vendor, KNOWN[i].product,
				KNOWN[i].why);
		}
	}

	gp_abilities_list_free (al);

	if (failures) {
		fprintf(stderr, "FAIL: %d undocumented ability PID collision(s)\n",
			failures);
		return EXIT_FAILURE;
	}
	if (stale) {
		fprintf(stderr, "FAIL: %d stale allowlist entr(ies)\n", stale);
		return EXIT_FAILURE;
	}

	printf("OK: no new ability PID collisions (%d rows checked, %d known "
	       "exception(s))\n", count, (int)(sizeof(KNOWN)/sizeof(KNOWN[0])));
	return EXIT_SUCCESS;
}
