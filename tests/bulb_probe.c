/* Supervised bulb open/close probe for Pentax K-1 II (research).
 *
 * Sequence:
 *  1. read conditions, log baseline
 *  2. initiate capture (0x9011) with focus_mode=2 (no AF)
 *  3. poll conditions until operation_state 49->50 or SHOOTING flag
 *  4. after --hold-ms, send terminate capture (0x9012, release_mode=0)
 *  5. poll conditions until idle again
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "gphoto2/gphoto2.h"
#include "ptp.h"
#include "ptp-private.h"
#include "pentax-utils.h"

static Camera *camera = NULL;
static GPContext *context = NULL;

static long now_ms(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

static void die(const char *msg, int ret) {
	fprintf(stderr, "FATAL: %s (%d)\n", msg, ret);
	exit(1);
}

/* returns 1 if shooting evidence seen */
static int dump_conditions(const char *label, int *saw_shooting, int *saw_state50) {
	PTPParams *params = &camera->pl->params;
	unsigned char *data = NULL;
	uint32_t size = 0;
	uint16_t ptpres;
	PentaxConditions cond;

	ptpres = ptp_pentax_get_all_conditions(params, &data, &size);
	if (ptpres != PTP_RC_OK) {
		printf("[%s] get_all_conditions failed: 0x%04x\n", label, ptpres);
		return 0;
	}
	if (pentax_parse_conditions(data, size, &cond) == GP_OK) {
		printf("[%s] t=%ldms state=%u flags=0x%x expmode=%u drive=%u "
			"bulb=%us/%u astro_limit=%u astro_flags=0x%x cap=0x%x\n",
			label, now_ms(), cond.operation_state, cond.activity_flags,
			cond.exposure_mode, cond.drive_mode,
			cond.bulb_timer_seconds, cond.bulb_timer_denominator,
			cond.astro_limit_seconds,
			cond.astro_status_flags, cond.capability_flags);
		if (cond.activity_flags & PENTAX_CONDITION_ACTIVITY_SHOOTING && saw_shooting)
			*saw_shooting = 1;
		if (cond.operation_state == 50 && saw_state50)
			*saw_state50 = 1;
	} else {
		printf("[%s] parse failed (size %u)\n", label, size);
	}
	free(data);
	return 0;
}

int main(int argc, char **argv) {
	int release_mode = argc > 3 ? atoi(argv[3]) : 0;
	long hold_ms = argc > 1 ? atol(argv[1]) : 1500;
	int ret;
	PTPParams *params;
	long t0, t;
	int saw_shooting = 0, saw_state50 = 0;

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
			if (argc > 2 && strcmp(v, argv[2]) == 0) idx = i;
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
		ret = gp_camera_set_port_info(camera, info);
		gp_port_info_list_free(pil);
		if (ret < GP_OK) die("set port info failed", ret);
		gp_list_free(list);
	}

	ret = gp_camera_init(camera, context);
	if (ret < GP_OK) die("camera init failed", ret);
	params = &camera->pl->params;
	printf("vendor_mode_enabled=%d\n", params->pentax.vendor_mode_enabled);

	dump_conditions("baseline", NULL, NULL);

	t0 = now_ms();
	printf(">>> initiating capture (release_mode=%d, focus_mode=2)\n", release_mode);
	fflush(stdout);
	uint16_t ptpres = ptp_pentax_initiate_capture(params, release_mode, 2, 0, 0, 0);
	printf("<<< initiate returned 0x%04x after %ldms\n", ptpres, now_ms() - t0);
	fflush(stdout);
	if (ptpres != PTP_RC_OK) { dump_conditions("post-init-fail", NULL, NULL); return 2; }

	while ((t = now_ms()) - t0 < hold_ms + 30000) {
		usleep(100000);
		dump_conditions("poll", &saw_shooting, &saw_state50);
		fflush(stdout);
		if (t - t0 >= hold_ms && (saw_shooting || saw_state50)) break;
		if (t - t0 >= hold_ms + 5000 && !saw_shooting && !saw_state50) {
			printf("no shooting evidence after hold+5s; terminating anyway\n");
			break;
		}
	}

	t0 = now_ms();
	printf(">>> sending terminate capture (release_mode=%d)\n", release_mode);
	fflush(stdout);
	ptpres = ptp_pentax_terminate_capture(params, release_mode);
	printf("<<< terminate returned 0x%04x after %ldms\n", ptpres, now_ms() - t0);
	fflush(stdout);

	for (int i = 0; i < 15; i++) {
		usleep(200000);
		dump_conditions("settle", NULL, NULL);
	}
	dump_conditions("final", NULL, NULL);

	gp_camera_exit(camera, context);
	return 0;
}
