/* Cross-process (0xd02c) + CI-mode (0xd020) probe for Pentax K-1 II.
 *
 * Usage: ./d02c_probe [port] [--dry-run | --value N]
 *   - reads the current d020 (CI mode) and d02c (cross process) values via
 *     ptp_pentax_get_device_prop_raw() and prints them
 *   - with --dry-run, stops after the reads (no writes at all)
 *   - with --value N (wire value 1..4 = user "N-1", or 33..64 = preset),
 *     performs the full HW verification sequence:
 *       0. read orig d020 + orig d02c
 *       1. SET d02c=N with d020 still at its original value (expected to be
 *          rejected by the camera unless it is already "cross process" — this
 *          step isolates the CI-mode precondition)
 *       2. SET d020=10 ("cross process") + read-back verify
 *       3. SET d02c=N + read-back verify (expected OK with mode active)
 *       4. restore d02c=orig, then d020=orig_ci_mode; read both back to
 *          confirm restoration
 *     Exits non-zero if any expected-OK step fails or a read-back mismatches.
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

/* Read a 1-byte device property; prints the raw PTP RC. */
static uint16_t get_u8(uint16_t prop, const char *label, uint8_t *out) {
	unsigned char *data = NULL;
	unsigned int size = 0;
	uint16_t rc = ptp_pentax_get_device_prop_raw(&camera->pl->params,
		prop, &data, &size);
	if (rc == PTP_RC_OK && size >= 1)
		*out = data[0];
	printf("GET %s -> raw PTP RC 0x%04x %s%s\n", label, rc,
	       rc == PTP_RC_OK ? "(OK)" : "",
	       (rc == PTP_RC_OK && size >= 1) ? "" : " [no data]");
	if (rc == PTP_RC_OK && size >= 1)
		printf("      %s = %u\n", label, *out);
	fflush(stdout);
	free(data);
	return rc;
}

/* Write a 1-byte device property; prints the raw PTP RC. */
static uint16_t set_u8(uint16_t prop, const char *label, uint8_t value) {
	unsigned char out[1] = { value };
	uint16_t rc = ptp_pentax_set_device_prop_raw(&camera->pl->params,
		prop, out, sizeof(out));
	printf("SET %s=%u -> raw PTP RC 0x%04x %s\n", label, (unsigned)value,
	       rc, rc == PTP_RC_OK ? "(OK)" : "");
	fflush(stdout);
	return rc;
}

int main(int argc, char **argv) {
	const char *port_arg = NULL;
	int dry_run = 0;
	int value = -1; /* wire value to write; -1 = none */
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--dry-run") == 0) dry_run = 1;
		else if (strcmp(argv[i], "--value") == 0 && i + 1 < argc)
			value = atoi(argv[++i]);
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
	printf("vendor_mode_enabled=%d\n", camera->pl->params.pentax.vendor_mode_enabled);

	uint8_t orig_ci = 0, orig_cp = 0;
	int cp_readable = 1; /* d02c GET may return 0x200a (mode-gated) */
	int ok = 1;

	/* Step 0: capture originals. d020 must be readable (we always restore
	 * it); an unreadable d02c is tolerated — its original is unknown and
	 * the restore step will use wire value 1 (user "0") instead. */
	if (get_u8(PTP_DPC_PENTAX_CustomImageMode, "d020 CI mode", &orig_ci) != PTP_RC_OK) {
		printf("RESULT: FAIL (could not read d020 original)\n");
		gp_camera_exit(camera, context);
		return 1;
	}
	if (get_u8(PTP_DPC_PENTAX_CustomImageCrossProcess, "d02c cross process", &orig_cp) != PTP_RC_OK) {
		cp_readable = 0;
		printf("      d02c original unknown (GET unsupported); restore will use wire value 1\n");
	}

	if (!dry_run && value >= 0) {
		uint8_t wire = (uint8_t)value;

		/* Step 1: write d02c with the ORIGINAL CI mode still active.
		 * Expected rejected unless orig_ci is already "cross process"
		 * (wire 10); either outcome is informative, so not fatal. */
		printf("-- precondition probe: d020 currently %u (%s) --\n",
		       (unsigned)orig_ci,
		       orig_ci == 10 ? "cross process" : "not cross process");
		uint16_t rc = set_u8(PTP_DPC_PENTAX_CustomImageCrossProcess,
			"d02c (precondition probe)", wire);
		if (rc != PTP_RC_OK) {
			printf("      precondition write rejected as expected "
			       "(camera requires CI mode 'cross process' first)\n");
		} else {
			uint8_t rb = 0;
			get_u8(PTP_DPC_PENTAX_CustomImageCrossProcess,
				"d02c read-back (precondition)", &rb);
			printf("      precondition write ACCEPTED with original "
			       "CI mode %u — precondition not enforced this run\n",
			       (unsigned)orig_ci);
		}

		/* Step 2: enable cross-process CI mode. */
		rc = set_u8(PTP_DPC_PENTAX_CustomImageMode, "d020 CI mode", 10);
		if (rc != PTP_RC_OK) ok = 0;
		usleep(500 * 1000); /* let the camera settle between commands */

		uint8_t rb_ci = 0;
		get_u8(PTP_DPC_PENTAX_CustomImageMode, "d020 read-back", &rb_ci);
		if (rb_ci != 10) {
			printf("      d020 read-back mismatch: expected 10, got %u\n",
			       (unsigned)rb_ci);
			ok = 0;
		}

		/* Step 3: write the cross-process value with mode active. */
		rc = set_u8(PTP_DPC_PENTAX_CustomImageCrossProcess, "d02c", wire);
		if (rc != PTP_RC_OK) ok = 0;
		usleep(500 * 1000);

		uint8_t rb_cp = 0;
		get_u8(PTP_DPC_PENTAX_CustomImageCrossProcess, "d02c read-back", &rb_cp);
		if (rb_cp != wire) {
			printf("      d02c read-back mismatch: expected %u, got %u\n",
			       (unsigned)wire, (unsigned)rb_cp);
			ok = 0;
		}

		/* Step 4: restore. d02c first (while cross-process mode is
		 * still active and accepts writes), then d020. If the original d02c
		 * was unreadable, restore to wire value 1 (user "0"). */
		uint8_t cp_restore = cp_readable ? orig_cp : 1;
		rc = set_u8(PTP_DPC_PENTAX_CustomImageCrossProcess, "d02c restore", cp_restore);
		if (rc != PTP_RC_OK) ok = 0;
		usleep(500 * 1000);

		rc = set_u8(PTP_DPC_PENTAX_CustomImageMode, "d020 CI mode restore", orig_ci);
		if (rc != PTP_RC_OK) ok = 0;
		usleep(500 * 1000);

		uint8_t rb_ci2 = 0, rb_cp2 = 0;
		get_u8(PTP_DPC_PENTAX_CustomImageMode, "d020 read-back (restored)", &rb_ci2);
		if (rb_ci2 != orig_ci) {
			printf("      restoration mismatch: d020 %u (want %u)\n",
			       (unsigned)rb_ci2, (unsigned)orig_ci);
			ok = 0;
		}
		uint16_t rc_cp2 = get_u8(PTP_DPC_PENTAX_CustomImageCrossProcess,
					  "d02c read-back (restored)", &rb_cp2);
		if (rc_cp2 == PTP_RC_OK && rb_cp2 != cp_restore) {
			printf("      restoration mismatch: d02c %u (want %u)\n",
			       (unsigned)rb_cp2, (unsigned)cp_restore);
			ok = 0;
		}

		printf("RESULT: %s\n", ok ? "PASS" : "FAIL");
	} else {
		printf("-- dry run: no writes performed --\n");
	}

	gp_camera_exit(camera, context);
	return ok ? 0 : 1;
}
