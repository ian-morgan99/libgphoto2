/* Real production startup bodies and types, mocked wire/USB operations.
 * No camera, port or libusb handle is opened by this program. */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gphoto2/gphoto2-library.h>
#include <gphoto2/gphoto2-port-log.h>
#include "i18n.h"
#include "ptp.h"
#include "ptp-private.h"
#include "pentax-utils.h"

#define CHECK(x) do { if (!(x)) { \
    fprintf (stderr, "FAIL case %u line %d: %s; operations=%s\n", \
        cases, __LINE__, #x, operations); exit (1); \
} } while (0)

static char operations[64];
static unsigned int cases, used, opened, observed;
static uint16_t open_results[16], close_result;
static int reset_result;
struct observation {
    uint16_t prop_result, conditions_result;
    uint8_t liveview;
    unsigned int size;
    uint32_t activity, candidate;
};
static struct observation observations[2];
static Camera camera;
static CameraPrivateLibrary private;
static GPPort port;
static unsigned int private_frees, params_frees;

static void record (char operation)
{
    CHECK (used + 1 < sizeof (operations));
    operations[used++] = operation;
    operations[used] = '\0';
}

uint16_t ptp_opensession (PTPParams *params, uint32_t session)
{
    CHECK (opened < 16 && session != 0);
    record ('O');
    return open_results[opened++];
}

uint16_t ptp_generic_no_data (PTPParams *params, uint16_t opcode,
    unsigned int count, ...)
{
    CHECK (opcode == PTP_OC_CloseSession && count == 0);
    record ('X');
    return close_result;
}

int gp_port_reset (GPPort *port)
{ CHECK (port->type == GP_PORT_USB); record ('R'); return reset_result; }

uint16_t ptp_usb_control_device_reset_request (PTPParams *params)
{ record ('D'); return PTP_RC_OK; }

unsigned int sleep (unsigned int seconds)
{ CHECK (seconds == 2); record ('S'); return 0; }

uint16_t ptp_getdevicepropvalue (PTPParams *params, uint32_t property,
    PTPPropValue *value, uint16_t datatype)
{
    CHECK (observed < 2);
    CHECK (property == PTP_DPC_PENTAX_UsbLiveViewMode && datatype == PTP_DTC_UINT8);
    record ('P');
    value->u8 = observations[observed].liveview;
    return observations[observed].prop_result;
}

static void put_u32 (unsigned char *p, uint32_t n)
{ p[0] = n; p[1] = n >> 8; p[2] = n >> 16; p[3] = n >> 24; }

uint16_t ptp_pentax_get_all_conditions (PTPParams *params,
    unsigned char **data, unsigned int *size)
{
    struct observation *o;
    CHECK (observed < 2);
    o = &observations[observed++];
    record ('C');
    /* Allocate even on failed reads: production must release every buffer. */
    *data = calloc (1, PENTAX_CONDITIONS_MIN_SIZE);
    CHECK (*data != NULL);
    *size = o->size;
    put_u32 (*data + 104, o->activity);
    put_u32 (*data + 36, o->candidate);
    return o->conditions_result;
}

const char *ptp_strerror (uint16_t result, uint16_t vendor) { return "mock"; }
void ptp_free_params (PTPParams *params) { params_frees++; }
void ptp_context_set (void *owner, GPContext *context) {}
int ptp_ptpip_disconnect (PTPParams *params) { record ('T'); return GP_OK; }

/* The fixture's private structure is static so flags remain inspectable
 * after failure. Real heap observation buffers still go through free(). */
static void fixture_free (void *p)
{ if (p == &private) private_frees++; else free (p); }
#define free fixture_free
#include "test-pentax-session.inc"
#undef free

static void fixture (uint16_t first)
{
    cases++;
    memset (&camera, 0, sizeof (camera));
    memset (&private, 0, sizeof (private));
    memset (&port, 0, sizeof (port));
    camera.pl = &private;
    camera.port = &port;
    port.type = GP_PORT_USB;
    private_frees = params_frees = 0;
#if defined(HAVE_ICONV) && defined(HAVE_LANGINFO_H)
    private.params.cd_ucs2_to_locale = private.params.cd_locale_to_ucs2 = (iconv_t)-1;
#endif
    used = opened = observed = 0;
    operations[0] = '\0';
    for (unsigned int i = 0; i < 16; i++) open_results[i] = PTP_RC_OK;
    open_results[0] = first;
    close_result = PTP_RC_OK;
    reset_result = GP_OK;
    memset (observations, 0, sizeof (observations));
    for (unsigned int i = 0; i < 2; i++) {
        observations[i].prop_result = observations[i].conditions_result = PTP_RC_OK;
        observations[i].size = PENTAX_CONDITIONS_MIN_SIZE;
    }
}

static void run (int pentax, int result, const char *expected)
{
    CHECK (tested_open_session (&camera, NULL, pentax) == result);
    CHECK (!strcmp (operations, expected));
    if (pentax && result < GP_OK) {
        CHECK (camera.pl == NULL && private_frees == 1 && params_frees == 1);
    } else {
        CHECK (camera.pl == &private && private_frees == 0 && params_frees == 0);
    }
}

int main (void)
{
    const uint16_t stale[] = {PTP_RC_SessionAlreadyOpened, 0x02fa,
        PTP_ERROR_RESP_EXPECTED, PTP_ERROR_IO};
    const uint32_t unsafe[] = {PENTAX_CONDITION_ACTIVITY_SHOOTING,
        PENTAX_CONDITION_ACTIVITY_PROCESSING, PENTAX_CONDITION_ACTIVITY_MOVIE_RECORDING,
        PENTAX_CONDITION_ACTIVITY_MIRROR_UPPING, PENTAX_CONDITION_ACTIVITY_INTERVAL_MODE,
        PENTAX_CONDITION_ACTIVITY_MULTI_CAPTURE, PENTAX_CONDITION_ACTIVITY_SELF_TIMER};

    fixture (PTP_RC_OK); run (1, GP_OK, "O");
    for (unsigned int i = 0; i < sizeof (stale) / sizeof (stale[0]); i++) {
        fixture (stale[i]); run (1, GP_OK, "OPCXRSOPC");
        for (unsigned int j = 0; j < sizeof (unsafe) / sizeof (unsafe[0]); j++) {
            fixture (stale[i]); observations[0].activity = unsafe[j];
            run (1, GP_ERROR_CAMERA_BUSY, "OPC");
            CHECK (private.params.pentax.recovery_required);
        }
        fixture (stale[i]); observations[0].conditions_result = PTP_ERROR_IO;
        run (1, GP_ERROR_CAMERA_BUSY, "OPC");
        fixture (stale[i]); observations[0].size = PENTAX_CONDITIONS_MIN_SIZE - 1;
        run (1, GP_ERROR_CAMERA_BUSY, "OPC");
        fixture (stale[i]); observations[0].prop_result = PTP_ERROR_IO;
        run (1, GP_ERROR_CAMERA_BUSY, "OPC");
        fixture (stale[i]); observations[0].liveview = 1;
        run (1, GP_ERROR_CAMERA_BUSY, "OPC");
        fixture (stale[i]); observations[0].liveview = 255;
        run (1, GP_ERROR_CAMERA_BUSY, "OPC");
        fixture (stale[i]); observations[0].candidate = 17;
        run (1, GP_ERROR_CAMERA_BUSY, "OPC");
        CHECK (private.params.pentax.candidate_handle == 17);
    }
    fixture (PTP_RC_SessionAlreadyOpened);
    private.params.pentax.recovery_required = 1;
    run (1, GP_ERROR_CAMERA_BUSY, "OPC"); /* Idle cannot erase an earlier barrier. */
    fixture (PTP_RC_SessionAlreadyOpened); port.type = GP_PORT_PTPIP;
    run (1, GP_ERROR_CAMERA_BUSY, "OT");
    fixture (PTP_RC_SessionAlreadyOpened); port.type = GP_PORT_USB_SCSI;
    run (1, GP_ERROR_CAMERA_BUSY, "O");
    fixture (PTP_RC_SessionAlreadyOpened); close_result = PTP_ERROR_TIMEOUT;
    run (1, GP_ERROR_TIMEOUT, "OPCX");
    fixture (PTP_RC_SessionAlreadyOpened); reset_result = GP_ERROR_IO;
    run (1, GP_ERROR_IO, "OPCXR");
    fixture (PTP_RC_SessionAlreadyOpened); open_results[1] = PTP_ERROR_TIMEOUT;
    run (1, GP_ERROR_TIMEOUT, "OPCXRSO");
    fixture (PTP_RC_SessionAlreadyOpened); open_results[1] = PTP_RC_SessionAlreadyOpened;
    run (1, translate_ptp_result (PTP_RC_SessionAlreadyOpened), "OPCXRSO");
    for (unsigned int i = 0; i < sizeof (unsafe) / sizeof (unsafe[0]); i++) {
        fixture (PTP_RC_SessionAlreadyOpened); observations[1].activity = unsafe[i];
        run (1, GP_ERROR_CAMERA_BUSY, "OPCXRSOPC");
        CHECK (private.params.pentax.recovery_required);
    }
    fixture (PTP_RC_SessionAlreadyOpened); observations[1].conditions_result = PTP_ERROR_IO;
    run (1, GP_ERROR_CAMERA_BUSY, "OPCXRSOPC");
    fixture (PTP_RC_SessionAlreadyOpened); observations[1].prop_result = PTP_ERROR_IO;
    run (1, GP_ERROR_CAMERA_BUSY, "OPCXRSOPC");
    fixture (PTP_RC_SessionAlreadyOpened); observations[1].candidate = 23;
    run (1, GP_ERROR_CAMERA_BUSY, "OPCXRSOPC");
    fixture (PTP_RC_SessionAlreadyOpened); observations[1].liveview = 1;
    run (1, GP_ERROR_CAMERA_BUSY, "OPCXRSOPC");
    fixture (PTP_RC_SessionAlreadyOpened);
    private.params.pentax.vendor_mode_unknown = 1;
    observations[0].conditions_result = PTP_ERROR_IO;
    run (1, GP_ERROR_CAMERA_BUSY, "OPC");
    CHECK (private.params.pentax.vendor_mode_unknown == 1);
    fixture (PTP_RC_InvalidTransactionID);
    for (unsigned int i = 0; i < 16; i++) open_results[i] = PTP_RC_InvalidTransactionID;
    run (1, translate_ptp_result (PTP_RC_InvalidTransactionID), "OOOOOOOOOO");

    /* Generic-camera branches keep their historical wire sequence. */
    fixture (PTP_RC_OK); run (0, GP_OK, "O");
    fixture (PTP_RC_SessionAlreadyOpened); run (0, GP_OK, "O");
    fixture (PTP_ERROR_IO); run (0, GP_OK, "ODO");
    fixture (PTP_ERROR_RESP_EXPECTED); open_results[1] = PTP_ERROR_RESP_EXPECTED;
    run (0, GP_OK, "ODODO");
    fixture (PTP_ERROR_IO); open_results[1] = open_results[2] = PTP_ERROR_IO;
    run (0, GP_ERROR_IO, "ODODO");
    fixture (PTP_ERROR_IO); port.type = GP_PORT_PTPIP;
    run (0, GP_OK, "OO");
    fixture (PTP_RC_InvalidTransactionID);
    for (unsigned int i = 0; i < 10; i++) open_results[i] = PTP_RC_InvalidTransactionID;
    run (0, GP_OK, "OOOOOOOOOODO");
    printf ("PASS: %u real startup-loop cases; guarded recovery and generic negative controls\n", cases);
    return 0;
}
