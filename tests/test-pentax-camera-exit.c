/* Compile the real camera_exit with mocked transport, never a USB session.
 * No production test hook and no duplicate cleanup implementation. */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gphoto2/gphoto2-library.h>
#include <gphoto2/gphoto2-port-log.h>
#include "ptp.h"
#include "ptp-private.h"
#include "ptp-bugs.h"
#include "pentax-utils.h"
#include "pentax-publication.h"
#ifdef HAVE_LIBWS232
#include <winsock2.h>
#endif

#define SET_CONTEXT_P(p, ctx) ptp_context_set ((p)->data, ctx)
#define CHECK(x) do { if (!(x)) { \
    fprintf (stderr, "FAIL line %d: %s\n", __LINE__, #x); exit (1); \
} } while (0)

static uint16_t operations[16];
static unsigned int operation_count;
static uint16_t restore_result, disable_result, close_result;
static uint32_t expected_model, expected_extension;
static int freed_enabled, freed_unknown, freed_params, cleared_publications;

static void record (uint16_t opcode)
{
    CHECK (operation_count < sizeof (operations) / sizeof (operations[0]));
    operations[operation_count++] = opcode;
}

static uint16_t pentax_restore_live_view (PTPParams *params, int force)
{
    CHECK (force == 1);
    record (PTP_DPC_PENTAX_UsbLiveViewMode);
    return restore_result;
}

uint16_t ptp_pentax_set_vendor_mode (PTPParams *params, uint32_t model,
    uint32_t enabled, uint32_t extension, uint32_t *flags)
{
    CHECK (model == expected_model && extension == expected_extension);
    CHECK (enabled == 0);
    record (PTP_OC_PENTAX_SetVendorMode);
    *flags = 0;
    return disable_result;
}

/* Retained so the test compiles against the old source and detects the bug. */
uint16_t ptp_pentax_camera_shutdown (PTPParams *params)
{
    record (PTP_OC_PENTAX_CameraShutdown);
    return PTP_RC_OK;
}

uint16_t ptp_generic_no_data (PTPParams *params, uint16_t opcode,
    unsigned int count, ...)
{
    record (opcode);
    CHECK (opcode == PTP_OC_CloseSession && count == 0);
    return close_result;
}

void ptp_context_set (void *owner, GPContext *context) {}
int pentax_capture_publications_clear (PTPParams *params)
{ cleared_publications++; return GP_OK; }
void ptp_free_params (PTPParams *params)
{
    freed_enabled = params->pentax.vendor_mode_enabled;
    freed_unknown = params->pentax.vendor_mode_unknown;
    freed_params++;
}
int ptp_get_one_event (PTPParams *params, PTPContainer *event) { return 0; }
uint16_t ptp_check_event (PTPParams *params) { return PTP_RC_OK; }
uint16_t ptp_check_eos_events (PTPParams *params) { return PTP_RC_OK; }
int camera_unprepare_capture (Camera *camera, GPContext *context)
{ return GP_OK; }
int ptp_ptpip_disconnect (PTPParams *params) { return GP_OK; }
const char *ptp_strerror (uint16_t result, uint16_t vendor) { return "mock"; }
uint16_t ptp_setdevicepropvalue (PTPParams *params, uint32_t prop,
    PTPPropValue *value, uint16_t datatype)
{ CHECK (0); return PTP_RC_GeneralError; }
uint16_t ptp_sony_9280 (PTPParams *params, uint32_t a, uint32_t b,
    uint32_t c, uint32_t d, uint32_t e, uint8_t f, uint8_t g)
{ CHECK (0); return PTP_RC_GeneralError; }
uint16_t ptp_sony_9281 (PTPParams *params, uint32_t a)
{ CHECK (0); return PTP_RC_GeneralError; }

#include "test-camera-exit.inc"

static Camera *fixture (uint32_t vendor, uint32_t model, uint32_t extension,
    int enabled, int unknown, int liveview, int dont_close)
{
    Camera *camera = NULL;
    PTPParams *params;
    CHECK (gp_camera_new (&camera) == GP_OK);
    camera->pl = calloc (1, sizeof (*camera->pl));
    CHECK (camera->pl != NULL);
    camera->port->type = GP_PORT_USB;
    params = &camera->pl->params;
    params->data = calloc (1, sizeof (PTPData));
    CHECK (params->data != NULL);
    params->deviceinfo.VendorExtensionID = vendor;
    params->pentax.model_no = expected_model = model;
    params->pentax.vendor_ext_version = expected_extension = extension;
    params->pentax.vendor_mode_enabled = enabled;
    params->pentax.vendor_mode_unknown = unknown;
    params->inliveview = liveview;
    params->device_flags = dont_close ? DEVICE_FLAG_DONT_CLOSE_SESSION : 0;
#if defined(HAVE_ICONV) && defined(HAVE_LANGINFO_H)
    params->cd_ucs2_to_locale = params->cd_locale_to_ucs2 = (iconv_t)-1;
#endif
    operation_count = 0;
    freed_params = cleared_publications = 0;
    freed_enabled = freed_unknown = -1;
    restore_result = disable_result = close_result = PTP_RC_OK;
    return camera;
}

static void finish (Camera *camera, int expected, int restore, int disable, int close)
{
    unsigned int i = 0;
    CHECK (camera_exit (camera, NULL) == expected);
    CHECK (camera->pl == NULL);
    CHECK (freed_params == 1 && cleared_publications == 1);
    CHECK (operation_count == (unsigned int)(restore + disable + close));
    if (restore) CHECK (operations[i++] == PTP_DPC_PENTAX_UsbLiveViewMode);
    if (disable) CHECK (operations[i++] == PTP_OC_PENTAX_SetVendorMode);
    if (close) CHECK (operations[i++] == PTP_OC_CloseSession);
    CHECK (gp_camera_unref (camera) == GP_OK);
}

int main (void)
{
    Camera *camera;
    /* Table covers every recognised modern model, including extension 0. */
    const uint32_t models[][2] = {
        {78400, 1}, {77970, 1}, {78420, 1}, {78550, 1}, {78380, 1},
        {78350, 1}, {77840, 1}, {77760, 1}, {77320, 0}, {78370, 1}, {78520, 1}
    };
    for (unsigned int i = 0; i < sizeof (models) / sizeof (models[0]); i++) {
        camera = fixture (PTP_VENDOR_PENTAX, models[i][0], models[i][1], 1, 0, 0, 0);
        finish (camera, GP_OK, 0, 1, 1);
        CHECK (freed_enabled == 0 && freed_unknown == 0);
    }
    camera = fixture (PTP_VENDOR_PENTAX, 78400, 1, 1, 0, 1, 0);
    camera->pl->params.pentax.keep_live_view = 1;
    finish (camera, GP_OK, 1, 1, 1);

    camera = fixture (PTP_VENDOR_PENTAX, 78400, 1, 0, 1, 0, 0);
    finish (camera, GP_OK, 0, 1, 1);
    CHECK (freed_enabled == 0 && freed_unknown == 0);

    camera = fixture (PTP_VENDOR_PENTAX, 78400, 1, 0, 1, 0, 0);
    disable_result = PTP_ERROR_IO;
    finish (camera, GP_ERROR_IO, 0, 1, 1);
    CHECK (freed_enabled == 0 && freed_unknown == 1);

    camera = fixture (PTP_VENDOR_PENTAX, 78400, 1, 1, 0, 0, 0);
    disable_result = PTP_ERROR_IO;
    finish (camera, GP_ERROR_IO, 0, 1, 1);
    CHECK (freed_enabled == 1 && freed_unknown == 1);

    camera = fixture (PTP_VENDOR_PENTAX, 78400, 1, 1, 0, 1, 0);
    restore_result = PTP_ERROR_TIMEOUT;
    disable_result = PTP_ERROR_IO;
    close_result = PTP_RC_GeneralError;
    finish (camera, GP_ERROR_TIMEOUT, 1, 1, 1);

    camera = fixture (PTP_VENDOR_PENTAX, 78400, 1, 1, 0, 1, 0);
    restore_result = PTP_ERROR_TIMEOUT;
    finish (camera, GP_ERROR_TIMEOUT, 1, 1, 1);

    camera = fixture (PTP_VENDOR_PENTAX, 78400, 1, 1, 0, 0, 0);
    close_result = PTP_ERROR_IO;
    finish (camera, GP_ERROR_IO, 0, 1, 1);

    camera = fixture (PTP_VENDOR_PENTAX, 78400, 1, 1, 0, 0, 0);
    disable_result = PTP_ERROR_TIMEOUT;
    close_result = PTP_ERROR_IO;
    finish (camera, GP_ERROR_TIMEOUT, 0, 1, 1);

    camera = fixture (PTP_VENDOR_PENTAX, 78400, 1, 0, 0, 0, 0);
    finish (camera, GP_OK, 0, 0, 1);
    camera = fixture (PTP_VENDOR_PENTAX, 78400, 1, 1, 0, 0, 1);
    finish (camera, GP_OK, 0, 1, 0);
    /* Enabled vendor mode wins over a generic DeviceInfo vendor ID. */
    camera = fixture (PTP_VENDOR_MICROSOFT, 78400, 1, 1, 0, 0, 0);
    finish (camera, GP_OK, 0, 1, 1);

    const uint32_t other_vendors[] = {PTP_VENDOR_CANON, PTP_VENDOR_NIKON,
        PTP_VENDOR_SONY, PTP_VENDOR_MICROSOFT};
    for (unsigned int i = 0; i < sizeof (other_vendors) / sizeof (other_vendors[0]); i++) {
        camera = fixture (other_vendors[i], 0, 0, 0, 0, 0, 0);
        close_result = PTP_ERROR_IO;
        finish (camera, GP_OK, 0, 0, 1); /* Historical non-Pentax semantics. */
        camera = fixture (other_vendors[i], 0, 0, 0, 0, 0, 1);
        finish (camera, GP_OK, 0, 0, 0);
    }
    CHECK (gp_camera_new (&camera) == GP_OK);
    CHECK (camera_exit (camera, NULL) == GP_OK); /* Partial/uninitialised cleanup. */
    CHECK (gp_camera_unref (camera) == GP_OK);
    puts ("PASS: real camera_exit, no shutdown, ordered cleanup, error precedence, non-Pentax controls");
    return 0;
}
