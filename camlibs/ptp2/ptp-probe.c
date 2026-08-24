/* ptp-probe: read-only PTP capability probe for Pentax bodies.
 *
 * Answers: which PTP operations does this device advertise, and how does it
 * react to Pentax vendor opcodes (0x90xx)?  Used to compare the K-01 (25fb:0131)
 * against K-3 III / K-70 etc.
 *
 * READ-ONLY by design: GetDeviceInfo, OpenSession/CloseSession, and
 * no-data probes of vendor opcodes with zero parameters.  No writes, no
 * capture, no property sets.
 *
 * Build (from libgphoto2/):
 *   cc -o _build/examples/ptp-probe camlibs/ptp2/ptp-probe.c \
 *     -Icamlibs/ptp2 -I_build -Ilibgphoto2_port/gphoto2 \
 *     -L_build/camlibs/ptp2 -lptp2 \
 *     -L_build/libgphoto2_port/libgphoto2_port -lgphoto2_port \
 *     -Wl,-rpath,$PWD/_build/camlibs/ptp2:$PWD/_build/libgphoto2_port/libgphoto2_port
 * Run:  ./_build/examples/ptp-probe usb:001,006
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "ptp.h"
#include <gphoto2/gphoto2-port.h>
#include <gphoto2/gphoto2-port-info-list.h>
#include <gphoto2/gphoto2-port-log.h>

static void probe_log_func (GPLogLevel level, const char *domain,
	const char *str, void *data)
{
	(void) level; (void) data;
	fprintf (stderr, "LOG %s: %s\n", domain ? domain : "?", str ? str : "");
}

/* Exported by the built ptp2.so (same symbols the camlib uses). */
extern uint16_t ptp_usb_sendreq (PTPParams *params, PTPContainer *req, int dataphase);
extern uint16_t ptp_usb_getresp (PTPParams *params, PTPContainer *ptp);
extern uint16_t ptp_usb_getdata (PTPParams *params, PTPContainer *ptp, PTPDataHandler *handler);
extern uint16_t ptp_generic_no_data (PTPParams *params, uint16_t code, unsigned int n_param, ...);
extern uint16_t ptp_getdeviceinfo (PTPParams *params, PTPDeviceInfo *deviceinfo);
extern void ptp_free_deviceinfo (PTPDeviceInfo *di);

/* The ptp_usb_* IO functions fetch the GPPort via
 * ((PTPData *)params->data)->camera->port.  We build a minimal stand-in
 * with the same leading layout instead of linking the whole camlib. */
typedef struct { GPPort *port; } FakeCamera;
typedef struct { FakeCamera *camera; void *context; } ProbePTPData;

static const char *rc_name (uint16_t rc)
{
	switch (rc) {
	case PTP_RC_OK: return "OK";
	case 0x5001: return "SessionNotSupported";
	case 0x5002: return "StorageFull";
	case 0x5003: return "InvalidHandle";
	case 0x5004: return "PartialDelete";
	case 0x5005: return "DeviceBusy";
	case 0x5006: return "DeviceOpen";
	case 0x5007: return "NotReady";
	case 0x5008: return "Unknown";
	case 0x5009: return "StorageNotReady";
	case 0x500a: return "NoMemoryCard";
	case 0x500b: return "InvalidParameter";
	case 0x5012: return "SessionAlreadyExists";
	case 0x5017: return "DevicePropNotSupported";
	case PTP_ERROR_BADPARAM: return "BADPARAM";
	case PTP_ERROR_RESP_EXPECTED: return "RESP_EXPECTED";
	case PTP_ERROR_IO: return "IO";
	default: return NULL;
	}
}

static void print_rc (const char *what, uint16_t rc)
{
	const char *n = rc_name (rc);
	printf ("  %-28s -> 0x%04x %s\n", what, rc, n ? n : "(vendor/other code)");
}

int main (int argc, char **argv)
{
	if (argc < 2) {
		fprintf (stderr, "usage: %s usb:BBB,DDD\n", argv[0]);
		return 2;
	}
	int busnum = 0, devnum = 0;
	if (sscanf (argv[1], "usb:%3x,%3x", &busnum, &devnum) != 2) {
		fprintf (stderr, "bad port string '%s' (want usb:BBB,DDD)\n", argv[1]);
		return 2;
	}
	if (getenv ("PROBE_DEBUG"))
		gp_log_add_func (GP_LOG_DEBUG, probe_log_func, NULL);

	GPPort *port = NULL;
	if (gp_port_new (&port) < 0 || !port) {
		fprintf (stderr, "gp_port_new failed\n");
		return 1;
	}
	char pathbuf[32];
	snprintf (pathbuf, sizeof (pathbuf), "usb:%03x,%03x", busnum, devnum);
	GPPortInfoList *il = NULL;
	if (gp_port_info_list_new (&il) < 0 || gp_port_info_list_load (il) < 0) {
		fprintf (stderr, "could not load port info list (IOLIBS set?)\n");
		return 1;
	}
	int idx = gp_port_info_list_lookup_path (il, pathbuf);
	if (idx < 0) {
		fprintf (stderr, "no USB port driver found for %s\n", pathbuf);
		return 1;
	}
	GPPortInfo info = NULL;
	if (gp_port_info_list_get_info (il, idx, &info) < 0 || !info) {
		fprintf (stderr, "could not get port info at index %d\n", idx);
		return 1;
	}
	if (gp_port_set_info (port, info) < 0) {
		fprintf (stderr, "gp_port_set_info failed\n");
		return 1;
	}
	gp_port_info_list_free (il);
	if (gp_port_open (port) < 0) {
		fprintf (stderr, "gp_port_open failed (is the camera on bus %d dev %d? gvfs holding it?)\n",
			busnum, devnum);
		return 1;
	}
	/* Re-apply settings now that the lowlevel is open: this triggers
	 * ops->update which issues SET_CONFIGURATION(config) and claims the
	 * interface with the detected altsetting — the same thing the gphoto2
	 * core achieves by calling find_device before open. */
	GPPortSettings settings;
	if (gp_port_get_settings (port, &settings) == GP_OK &&
	    gp_port_set_settings (port, settings) != GP_OK) {
		fprintf (stderr, "gp_port_set_settings failed\n");
		return 1;
	}
	printf ("Port opened: usb:%03x,%03x (maxpacketsize=%d)\n",
		busnum, devnum, port->settings.usb.maxpacketsize);

	PTPParams params;
	memset (&params, 0, sizeof (params));
	params.byteorder = PTP_DL_LE;
	params.maxpacketsize = port->settings.usb.maxpacketsize ? port->settings.usb.maxpacketsize : 64;
	params.sendreq_func = ptp_usb_sendreq;
	params.getresp_func = ptp_usb_getresp;
	params.getdata_func = ptp_usb_getdata;
	FakeCamera fake_camera = { port };
	ProbePTPData probe_data = { &fake_camera, NULL };
	params.data = &probe_data;

	/* --- 1. GetDeviceInfo (works without a session) --- */
	printf ("\n== GetDeviceInfo ==\n");
	PTPDeviceInfo di;
	memset (&di, 0, sizeof (di));
	uint16_t rc = ptp_getdeviceinfo (&params, &di);
	if (rc != PTP_RC_OK) {
		print_rc ("GetDeviceInfo", rc);
		printf ("Device does not answer standard PTP GetDeviceInfo.\n");
	} else {
		printf ("  Manufacturer:      %s\n", di.Manufacturer ? di.Manufacturer : "(none)");
		printf ("  Model:             %s\n", di.Model ? di.Model : "(none)");
		printf ("  DeviceVersion:     %s\n", di.DeviceVersion ? di.DeviceVersion : "(none)");
		printf ("  SerialNumber:      %s\n", di.SerialNumber ? di.SerialNumber : "(none)");
		printf ("  StandardVersion:   0x%04x\n", di.StandardVersion);
		printf ("  VendorExtensionID: 0x%08x (Pentax=0x0000000D)\n", di.VendorExtensionID);
		printf ("  VendorExtVersion:  0x%04x\n", di.VendorExtensionVersion);
		printf ("  FunctionalMode:    0x%04x\n", di.FunctionalMode);
		printf ("  Operations (%u):\n", (unsigned) di.Operations_len);
		for (uint32_t i = 0; i < di.Operations_len; i++) {
			uint16_t op = di.Operations[i];
			const char *tag = "";
			if (op >= 0x9000 && op <= 0x9fff) tag = "  <-- PENTAX VENDOR";
			else if (op >= 0x8000 && op <= 0x8fff) tag = "  <-- MTP vendor";
			printf ("    0x%04x%s\n", op, tag);
		}
		printf ("  Events (%u):", (unsigned) di.Events_len);
		for (uint32_t i = 0; i < di.Events_len; i++)
			printf (" 0x%04x", di.Events[i]);
		printf ("\n");
		printf ("  DeviceProps (%u):", (unsigned) di.DeviceProps_len);
		for (uint32_t i = 0; i < di.DeviceProps_len; i++)
			printf (" 0x%04x", di.DeviceProps[i]);
		printf ("\n");
		ptp_free_deviceinfo (&di);
	}

	/* --- 2. OpenSession --- */
	printf ("\n== OpenSession ==\n");
	int session_open = 0;
	rc = ptp_generic_no_data (&params, PTP_OC_OpenSession, 1, 0);
	print_rc ("OpenSession", rc);
	if (rc == PTP_RC_OK) {
		session_open = 1;
		printf ("  session established\n");
	}

	/* --- 3. Probe Pentax vendor opcodes (no-data, zero params).
	 * A device that knows the opcode answers with a PTP error code
	 * (e.g. InvalidParameter 0x500b); a device that does not know it
	 * answers Unknown (0x5008) or times out (IO).  Zero parameters are
	 * safe: SetVendorMode(0,0,0,0) is a disable; GetAllConditions(0)
	 * reads conditions; the rest are read-only queries. --- */
	printf ("\n== Pentax vendor opcode probe (no-data, zero params) ==\n");
	struct { const char *name; uint16_t code; } probes[] = {
		{ "SetVendorMode(0x9001)", 0x9001 },
		{ "GetAllConditions(0x900f)", 0x900f },
		{ "FocusControl(0x9016)", 0x9016 },
		{ "FocusControlNew(0x9017)", 0x9017 },
	};
	for (size_t i = 0; i < sizeof (probes) / sizeof (probes[0]); i++) {
		rc = ptp_generic_no_data (&params, probes[i].code, 0);
		print_rc (probes[i].name, rc);
	}

	/* --- 4. CloseSession --- */
	if (session_open) {
		printf ("\n== CloseSession ==\n");
		rc = ptp_generic_no_data (&params, PTP_OC_CloseSession, 0);
		print_rc ("CloseSession", rc);
	}

	gp_port_close (port);
	gp_port_free (port);
	printf ("\nDone. (read-only probe; no settings changed)\n");
	return 0;
}
