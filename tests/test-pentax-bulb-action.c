/* Issue #140: executable regression for the Pentax held-Bulb action's cancellation
 * and stop-edge state machine.
 *
 * The static contract test (tests/test-pentax-capture-contract.py) asserts source
 * substrings and ordering. It cannot show that a cancel actually returns
 * GP_ERROR_CANCEL without writing a shutter, nor that pending state is cleared so the
 * next capture is permitted. Commit 4fba11d81 added exactly such a guard; a v75 shutter
 * readback change once passed every offline gate and then physically hung every timed
 * capture, which is the gap this closes.
 *
 * The function under test is ptp2_pentax_bulb_action, extracted verbatim from
 * camlibs/ptp2/library.c into test-pentax-bulb-action.inc -- no duplicated logic and no
 * test-only hook in the camlib. pentax-utils.c is compiled directly, so the model gate,
 * the hold-to-seconds conversion and the whole-second wire layout are the production
 * implementations rather than paraphrases of them.
 *
 * Only the transport, the capture path and the core abilities accessor are mocked, and
 * the clock is scripted: the measured Bulb hold therefore produces an exact shutter
 * value instead of one that depends on wall-clock rounding between two clock reads.
 *
 * NOT hardware qualification. This proves control flow, opcode order and state
 * transitions against a fake transport; it says nothing about how a real K-1 II or
 * K-3 III responds. Physical follow-up is patcher #261.
 */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gphoto2/gphoto2-library.h>
#include <gphoto2/gphoto2-port-log.h>
#include "i18n.h"
#include "ptp.h"
#include "ptp-private.h"
#include "pentax-utils.h"

/* The function body is wrapped in #ifndef LIBGPHOTO2_ENABLE_PENTAX_RESEARCH_CAPTURE,
 * which makes it return GP_ERROR_NOT_SUPPORTED when the flag is unset. config.h defines
 * the macro to 0 whenever the meson pentax_research_capture option is off -- and an
 * #ifndef guard would not catch that, because the macro *is* defined, just falsey. Left
 * alone, this harness would silently compile the stub and assert against a function that
 * does nothing. Force the real path on: the appliance build passes
 * -DLIBGPHOTO2_ENABLE_PENTAX_RESEARCH_CAPTURE=1 (container/build_ptp2.sh), so this is
 * the code that actually ships. */
#undef LIBGPHOTO2_ENABLE_PENTAX_RESEARCH_CAPTURE
#define LIBGPHOTO2_ENABLE_PENTAX_RESEARCH_CAPTURE 1

#define CHECK(x) do { if (!(x)) { \
    fprintf (stderr, "FAIL case %u line %d: %s; operations=%s\n", \
        cases, __LINE__, #x, operations); exit (1); \
} } while (0)

/* S = shutter property write, C = capture path entered, T = TerminateCapture. */
static char operations[64];
static unsigned int cases, used;
static uint64_t written_shutter;
static int capture_modes[4], capture_calls;
static uint16_t terminate_result;
static int terminate_calls;

/* Scripted clock: the start edge is stamped at 1000 ms and "now" at 25500 ms, so the
 * measured hold is exactly 25 s and the expected wire value is (1<<32)|25. */
#define START_MS 1000ULL
#define NOW_MS   25500ULL
#define EXPECTED_SECONDS 25U
#define EXPECTED_SHUTTER (((uint64_t) 1 << 32) | EXPECTED_SECONDS)

static uint64_t pentax_monotonic_ms (void) { return NOW_MS; }

static Camera camera;
static CameraPrivateLibrary private;
static GPContext *context;
static int cancel_answer;

static void record (char operation)
{
    CHECK (used + 1 < sizeof (operations));
    operations[used++] = operation;
    operations[used] = '\0';
}

/* The core implementation of gp_camera_get_abilities() is a memcpy out of the private
 * core structure, which is not part of the public headers. Supplying the accessor here
 * keeps the fixture honest about what the driver actually reads -- the USB product id --
 * without reaching into a private layout that could change underneath us. A definition
 * in the executable takes precedence over the one in the shared library at link time. */
int gp_camera_get_abilities (Camera *cam, CameraAbilities *a)
{
    CHECK (cam == &camera && a != NULL);
    memset (a, 0, sizeof (*a));
    a->usb_product = 0x0189;          /* K-3 III: the qualified held-Bulb body */
    return GP_OK;
}

uint16_t ptp_setdevicepropvalue (PTPParams *params, uint32_t property,
    PTPPropValue *value, uint16_t datatype)
{
    CHECK (property == PTP_DPC_PENTAX_ShutterSpeed);
    CHECK (datatype == PTP_DTC_UINT64);
    record ('S');
    written_shutter = value->u64;
    return PTP_RC_OK;
}

uint16_t ptp_pentax_terminate_capture (PTPParams *params, uint32_t mode)
{
    (void) params; (void) mode;
    record ('T');
    terminate_calls++;
    return terminate_result;
}

/* Records which capture mode was entered so the harness can tell a deferred
 * camera-timed fallback from a held-Bulb start or a finalize.
 *
 * Forward-declared with an int parameter and defined after the extracted body is
 * included: PentaxCaptureMode is defined by that body, so naming it here would be a use
 * before definition. The enum promotes to int at the call site, and the assertions below
 * compare against the production enumerators once they are in scope. */
static int camera_pentax_capture_internal (Camera *cam, CameraFilePath *path,
    GPContext *ctx, int mode);

static GPContextFeedback fake_cancel (GPContext *ctx, void *data)
{
    (void) ctx; (void) data;
    return cancel_answer ? GP_CONTEXT_FEEDBACK_CANCEL : GP_CONTEXT_FEEDBACK_OK;
}

/* The extracted body is included after the mocks so its calls bind to them. */
#include "test-pentax-bulb-action.inc"

static int camera_pentax_capture_internal (Camera *cam, CameraFilePath *path,
    GPContext *ctx, int mode)
{
    (void) cam; (void) ctx;
    CHECK (capture_calls < 4);
    record ('C');
    capture_modes[capture_calls++] = mode;
    return GP_OK;
}

static void fixture (int action_active, uint32_t transfer_state)
{
    cases++;
    memset (&camera, 0, sizeof (camera));
    memset (&private, 0, sizeof (private));
    memset (operations, 0, sizeof (operations));
    used = capture_calls = terminate_calls = 0;
    written_shutter = 0;
    cancel_answer = 0;
    terminate_result = PTP_RC_OK;
    camera.pl = &private;
    private.params.pentax.supported_model = 1;
    private.params.pentax.vendor_mode_enabled = 1;
    private.params.pentax.model_no = PENTAX_MODEL_K3_MARK_III;
    private.params.pentax.bulb_action_active = action_active;
    private.params.pentax.transfer_state = transfer_state;
}

/* Requirement 2: cancellation before initiation returns GP_ERROR_CANCEL, sends no
 * shutter write and no capture, and clears the deferred-fallback record. This is the
 * exact behaviour commit 4fba11d81 added; against the pre-fix source this case fails
 * red because the shutter is written and a capture is started. */
static void test_cancel_before_initiation (void)
{
    fixture (0, PTP_PENTAX_TRANSFER_TRIGGERED);
    private.params.pentax.bulb_action_timed_fallback = 1;
    private.params.pentax.bulb_action_start_ms = START_MS;
    private.params.pentax.bulb_action_body_reported_seconds = 7;
    cancel_answer = 1;

    CHECK (ptp2_pentax_bulb_action (&camera, 0, context) == GP_ERROR_CANCEL);
    CHECK (strcmp (operations, "") == 0);          /* no shutter, no capture */
    CHECK (private.params.pentax.bulb_action_timed_fallback == 0);
    CHECK (private.params.pentax.bulb_action_start_ms == 0);
    CHECK (private.params.pentax.bulb_action_body_reported_seconds == 0);
}

/* Requirement 3: without cancellation the camera-timed path runs exactly once, writes
 * the shutter derived from the measured hold, and does not send TerminateCapture. */
static void test_no_cancel_runs_once (void)
{
    fixture (0, PTP_PENTAX_TRANSFER_TRIGGERED);
    private.params.pentax.bulb_action_timed_fallback = 1;
    private.params.pentax.bulb_action_start_ms = START_MS;

    CHECK (ptp2_pentax_bulb_action (&camera, 0, context) == GP_OK);
    CHECK (strcmp (operations, "SC") == 0);        /* shutter then capture, nothing else */
    CHECK (capture_calls == 1 && capture_modes[0] == PENTAX_CAPTURE_TIMED);
    CHECK (written_shutter == EXPECTED_SHUTTER);   /* 25 s hold, whole-second layout */
    CHECK (terminate_calls == 0);                  /* no unnecessary TerminateCapture */
    CHECK (private.params.pentax.bulb_action_timed_fallback == 0);
}

/* Requirement 5: a null context must not crash. gp_context_cancel() returns OK for
 * NULL, so the fallback proceeds normally. */
static void test_null_context_proceeds (void)
{
    fixture (0, PTP_PENTAX_TRANSFER_TRIGGERED);
    private.params.pentax.bulb_action_timed_fallback = 1;
    private.params.pentax.bulb_action_start_ms = START_MS;

    CHECK (ptp2_pentax_bulb_action (&camera, 0, NULL) == GP_OK);
    CHECK (strcmp (operations, "SC") == 0);
    CHECK (written_shutter == EXPECTED_SHUTTER);
}

/* Requirement 2 again, from the consumer's side: after a cancel the pending record is
 * gone, so an immediate new start edge is not blocked by leftover state. */
static void test_cancel_leaves_session_usable (void)
{
    fixture (0, PTP_PENTAX_TRANSFER_TRIGGERED);
    private.params.pentax.bulb_action_timed_fallback = 1;
    private.params.pentax.bulb_action_start_ms = START_MS;
    cancel_answer = 1;
    CHECK (ptp2_pentax_bulb_action (&camera, 0, context) == GP_ERROR_CANCEL);

    /* A fresh start edge on the same session must be admitted. */
    private.params.pentax.transfer_state = PTP_PENTAX_TRANSFER_IDLE;
    cancel_answer = 0;
    CHECK (ptp2_pentax_bulb_action (&camera, 1, context) == GP_OK);
    CHECK (capture_modes[capture_calls - 1] == PENTAX_CAPTURE_BULB_START);
}

/* A start edge that finds an unclaimed fallback from a previous start discards it
 * rather than acting on an unrelated timestamp. */
static void test_start_discards_stale_fallback (void)
{
    fixture (0, PTP_PENTAX_TRANSFER_IDLE);
    private.params.pentax.bulb_action_timed_fallback = 1;
    private.params.pentax.bulb_action_start_ms = START_MS;

    CHECK (ptp2_pentax_bulb_action (&camera, 1, context) == GP_OK);
    CHECK (strcmp (operations, "C") == 0);         /* straight to the start edge */
    CHECK (capture_modes[0] == PENTAX_CAPTURE_BULB_START);
    CHECK (private.params.pentax.bulb_action_timed_fallback == 0);
}

/* Requirement 5: busy conditions reject before any shutter or capture. */
static void test_start_rejected_when_busy (void)
{
    fixture (0, PTP_PENTAX_TRANSFER_TRIGGERED);
    private.params.pentax.capture_output_pending = 1;
    CHECK (ptp2_pentax_bulb_action (&camera, 1, context) == GP_ERROR_CAMERA_BUSY);
    CHECK (strcmp (operations, "") == 0);

    fixture (1, PTP_PENTAX_TRANSFER_IDLE);         /* an action already held */
    CHECK (ptp2_pentax_bulb_action (&camera, 1, context) == GP_ERROR_CAMERA_BUSY);
    CHECK (strcmp (operations, "") == 0);
}

/* Requirement 4: a stop on an active held Bulb takes the TerminateCapture path and
 * then finalizes, in that order. */
static void test_stop_terminates_then_finalizes (void)
{
    fixture (1, PTP_PENTAX_TRANSFER_TRIGGERED);
    CHECK (ptp2_pentax_bulb_action (&camera, 0, context) == GP_OK);
    CHECK (strcmp (operations, "TC") == 0);
    CHECK (capture_modes[0] == PENTAX_CAPTURE_BULB_FINALIZE);
    CHECK (private.params.pentax.bulb_action_stop_requested == 1);
    CHECK (private.params.pentax.recovery_required == 0);
}

/* A rejected TerminateCapture must not be replayed and must ask for recovery. */
static void test_stop_rejected_is_not_replayed (void)
{
    fixture (1, PTP_PENTAX_TRANSFER_TRIGGERED);
    terminate_result = PTP_RC_DeviceBusy;

    CHECK (ptp2_pentax_bulb_action (&camera, 0, context) == GP_ERROR_CAMERA_BUSY);
    CHECK (strcmp (operations, "T") == 0);         /* no capture entered */
    CHECK (terminate_calls == 1);
    CHECK (private.params.pentax.bulb_action_stop_requested == 1);
    CHECK (private.params.pentax.recovery_required == 1);
}

/* Requirement 4's stop-vs-natural-completion race: the camera closed the exposure on
 * its own, so 0x2018 is not an error and output reconciliation still runs. */
static void test_stop_already_terminated_continues (void)
{
    fixture (1, PTP_PENTAX_TRANSFER_TRIGGERED);
    terminate_result = PTP_RC_CaptureAlreadyTerminated;

    CHECK (ptp2_pentax_bulb_action (&camera, 0, context) == GP_OK);
    CHECK (strcmp (operations, "TC") == 0);
    CHECK (capture_modes[0] == PENTAX_CAPTURE_BULB_FINALIZE);
    CHECK (private.params.pentax.recovery_required == 0);
}

/* A stop with no matching active start must not send another shutter command. */
static void test_stop_without_start_refused (void)
{
    fixture (0, PTP_PENTAX_TRANSFER_IDLE);         /* nothing held, no fallback */
    CHECK (ptp2_pentax_bulb_action (&camera, 0, context) == GP_ERROR_CAMERA_BUSY);
    CHECK (strcmp (operations, "") == 0);

    fixture (1, PTP_PENTAX_TRANSFER_TRIGGERED);    /* already stopped */
    private.params.pentax.bulb_action_stop_requested = 1;
    CHECK (ptp2_pentax_bulb_action (&camera, 0, context) == GP_ERROR_CAMERA_BUSY);
    CHECK (strcmp (operations, "") == 0);
}

/* Requirement 6: the control case. Outside vendor mode, or on a model kept out of the
 * qualified set, the action must refuse without touching the transport -- so no other
 * vendor or personality can be affected by this path. */
static void test_non_pentax_control (void)
{
    fixture (1, PTP_PENTAX_TRANSFER_TRIGGERED);
    private.params.pentax.vendor_mode_enabled = 0;
    CHECK (ptp2_pentax_bulb_action (&camera, 1, context) == GP_ERROR_NOT_SUPPORTED);
    CHECK (strcmp (operations, "") == 0);

    fixture (1, PTP_PENTAX_TRANSFER_TRIGGERED);
    private.params.pentax.supported_model = 0;
    CHECK (ptp2_pentax_bulb_action (&camera, 0, context) == GP_ERROR_NOT_SUPPORTED);
    CHECK (strcmp (operations, "") == 0);

    /* The K-3 III Monochrome is deliberately held out of the qualified set. */
    fixture (1, PTP_PENTAX_TRANSFER_TRIGGERED);
    private.params.pentax.model_no = PENTAX_MODEL_K3_MARK_III_MONOCHROME;
    CHECK (ptp2_pentax_bulb_action (&camera, 1, context) == GP_ERROR_NOT_SUPPORTED);
    CHECK (strcmp (operations, "") == 0);
}

int main (void)
{
    context = gp_context_new ();
    CHECK (context != NULL);
    gp_context_set_cancel_func (context, fake_cancel, NULL);

    test_cancel_before_initiation ();
    test_no_cancel_runs_once ();
    test_null_context_proceeds ();
    test_cancel_leaves_session_usable ();
    test_start_discards_stale_fallback ();
    test_start_rejected_when_busy ();
    test_stop_terminates_then_finalizes ();
    test_stop_rejected_is_not_replayed ();
    test_stop_already_terminated_continues ();
    test_stop_without_start_refused ();
    test_non_pentax_control ();

    gp_context_unref (context);
    printf ("PASS: %u Pentax held-Bulb action cases against a mocked transport. "
        "NOT hardware qualified.\n", cases);
    fflush (stdout);
    return 0;
}
