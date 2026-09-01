/* Card-writing-mode (0x9004) + Green button (0x9013) probe for Pentax K-1 II.
 *
 * Usage: ./cardwrite_probe [port] [--green]
 *   - calls ptp_pentax_set_card_writing_mode() for modes 0..3 and prints the
 *     raw PTP RC (distinguishes camera-side rejection from port-layer errors)
 *   - with --green, also fires ptp_pentax_interrupt() (Green button, no params)
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

int main(int argc, char **argv) {
	const char *port_arg = NULL;
	int do_green = 0;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--green") == 0) do_green = 1;
		else if (!port_arg) port_arg = argv[i];
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

	static const char *names[4] = { "off(0)", "SD1(1)", "SD2(2)", "both(3)" };
	for (uint32_t mode = 0; mode <= 3; mode++) {
		uint16_t rc = ptp_pentax_set_card_writing_mode(params, mode);
		printf("SET cardwritingmode=%s -> raw PTP RC 0x%04x %s\n",
		       names[mode], rc, rc == PTP_RC_OK ? "(OK)" : "");
		fflush(stdout);
		usleep(500 * 1000); /* let the camera settle between commands */
	}

	if (do_green) {
		uint16_t rc = ptp_pentax_interrupt(params);
		printf("Green button interrupt 0x9013 -> raw PTP RC 0x%04x %s\n",
		       rc, rc == PTP_RC_OK ? "(OK)" : "");
		fflush(stdout);
	}

	gp_camera_exit(camera, context);
	return 0;
}
