/* Cross-process (d02c) + user filter (d02d) raw property probe for Pentax K-1 II.
 *
 * Usage: ./crossprocess_probe [port]
 *   - READ-ONLY: issues standard PTP GetDevicePropValue / GetDevicePropDesc on
 *     0xd02c and 0xd02d (no Set, no vendor commands) and prints the raw bytes
 *     plus a best-effort parse of the property descriptor (form, datatype,
 *     access capability, category, int32 min/max/default or enum values).
 *   - gp_camera_init() enables Pentax vendor mode first (library.c), so these
 *     properties are queried in the same state as normal gphoto2 sessions.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "gphoto2/gphoto2.h"
#include "ptp.h"
#include "ptp-private.h"

static Camera *camera = NULL;
static GPContext *context = NULL;

static void die(const char *msg, int ret) {
	fprintf(stderr, "FATAL: %s (%d)\n", msg, ret);
	exit(1);
}

static uint32_t le32(const unsigned char *p) {
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
	       ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t le16(const unsigned char *p) {
	return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static void hexdump(const char *prefix, const unsigned char *data, unsigned int size) {
	printf("%s", prefix);
	for (unsigned int i = 0; i < size && i < 64; i++)
		printf(" %02x", data[i]);
	if (size > 64)
		printf(" ...");
	printf("\n");
}

static void dump_prop(PTPParams *params, const char *label, uint16_t propcode) {
	unsigned char *data = NULL;
	unsigned int size = 0;
	uint16_t rc = ptp_pentax_get_device_prop_raw(params, propcode, &data, &size);
	if (rc != PTP_RC_OK) {
		printf("%s: GET value -> RC 0x%04x\n", label, rc);
	} else if (size == 0) {
		printf("%s: GET value -> RC OK but EMPTY data phase\n", label);
	} else {
		hexdump(label, data, size);
		if (size >= 4) {
			uint32_t v = le32(data);
			printf("    (LE u32 interpretation: %u / 0x%08x)\n", v, v);
		}
		free(data);
	}

	unsigned char *desc = NULL;
	unsigned int dsize = 0;
	uint16_t drc = ptp_pentax_get_device_prop_desc_raw(params, propcode, &desc, &dsize);
	if (drc != PTP_RC_OK) {
		printf("%s: GET desc -> RC 0x%04x\n", label, drc);
	} else if (dsize == 0) {
		printf("%s: GET desc -> RC OK but EMPTY data phase\n", label);
	} else {
		hexdump(label, desc, dsize);
		if (dsize >= 8) {
			uint16_t form = le16(desc), dtype = le16(desc + 2);
			uint16_t access = le16(desc + 4), cat = le16(desc + 6);
			printf("    (form=%u datatype=%u access=0x%04x category=%u%s)\n",
			       form, dtype, access, cat,
			       (access & 0x8000) ? " [read-only]" :
			       ((access & 0x2000) && !(access & 0x1000)) ? " [gettable]" : "");
			if (dtype == 1 && dsize >= 20) { /* INT32 */
				int32_t mn = (int32_t)le32(desc + 8);
				int32_t mx = (int32_t)le32(desc + 12);
				int32_t df = (int32_t)le32(desc + 16);
				printf("    (int32 min=%d max=%d default=%d)\n", mn, mx, df);
			} else if ((dtype == 4 || form == 4) && dsize >= 12) { /* ENUM */
				uint32_t numvals = le32(desc + 8);
				printf("    (enum values:");
				for (uint32_t i = 0; i < numvals && i < 16 && 12 + 4 * (i + 1) <= dsize; i++)
					printf(" %u", le32(desc + 12 + 4 * i));
				if (numvals > 16)
					printf(" ...");
				printf(")\n");
			} else if ((dtype == 2 || form == 0) && dsize >= 12) { /* STRING */
				uint32_t maxlen = le32(desc + 8);
				printf("    (string maxLen=%u)\n", maxlen);
			}
		}
		free(desc);
	}
}

int main(int argc, char **argv) {
	const char *port_arg = NULL;
	for (int i = 1; i < argc; i++) {
		if (!port_arg && argv[i][0] != '-')
			port_arg = argv[i];
	}

	printf("probe pid=%d\n", getpid());

	context = gp_context_new();

	{
		CameraList *list;
		const char *model = NULL, *port = NULL;
		gp_list_new(&list);
		if (gp_camera_autodetect(list, context) < GP_OK || gp_list_count(list) < 1)
			die("no camera autodetected", 0);
		int idx = 0;
		for (int i = 0; i < gp_list_count(list); i++) {
			const char *v = NULL;
			gp_list_get_value(list, i, &v);
			if (port_arg && strcmp(v, port_arg) == 0) idx = i;
		}
		gp_list_get_name(list, idx, &model);
		gp_list_get_value(list, idx, &port);
		printf("detected: %s @ %s\n", model, port);

		CameraAbilitiesList *al;
		gp_abilities_list_new(&al);
		gp_abilities_list_load(al, context);
		int n = gp_abilities_list_lookup_model(al, model);
		if (n < 0) die("model lookup failed", n);
		CameraAbilities a;
		gp_abilities_list_get_abilities(al, n, &a);
		gp_abilities_list_free(al);
		gp_camera_new(&camera);
		gp_camera_set_abilities(camera, a);

		GPPortInfoList *pil;
		gp_port_info_list_new(&pil);
		gp_port_info_list_load(pil);
		int i = gp_port_info_list_lookup_path(pil, port);
		if (i < 0) die("port lookup failed", i);
		GPPortInfo info = NULL;
		gp_port_info_list_get_info(pil, i, &info);
		int ret = gp_camera_set_port_info(camera, info);
		gp_port_info_list_free(pil);
		if (ret < GP_OK) die("set port info failed", ret);
		gp_list_free(list);
	}

	int ret = gp_camera_init(camera, context);
	if (ret < GP_OK) die("camera init failed", ret);
	PTPParams *params = &camera->pl->params;
	printf("vendor_mode_enabled=%d\n", params->pentax.vendor_mode_enabled);

	dump_prop(params, "crossprocess d02c:", 0xd02c);
	dump_prop(params, "userfilter   d02d:", 0xd02d);

	gp_camera_exit(camera, context);
	return 0;
}
