/* Pin down what PTP failure codes look like to a client.
 *
 * These codes used to fall through to the default branch and be reported as a
 * generic GP_ERROR, with only a debug line naming the code. A caller then saw -1
 * for all of them and could not tell an unsupported property from a bad value
 * from a full card. Observed live on a K-1 II: GetDevicePropDesc answered
 * PTP_RC_DevicePropNotSupported (0x200a) and the log said "unmapped PTP result
 * code 0x200a reported as generic error".
 *
 * The production body is extracted, not reimplemented, so this cannot pass by
 * asserting a copy of itself. */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <gphoto2/gphoto2-library.h>
#include <gphoto2/gphoto2-port-log.h>
#include "ptp.h"

#define CHECK(x) do { if (!(x)) { \
    fprintf (stderr, "FAIL line %d: %s\n", __LINE__, #x); exit (1); \
} } while (0)

int translate_ptp_result (uint16_t result);
#include "test-ptp-result-mapping.inc"

static void expect (uint16_t ptp, int want, const char *why)
{
    int got = translate_ptp_result (ptp);
    if (got != want) {
        fprintf (stderr, "FAIL 0x%04x -> %d, wanted %d (%s)\n",
            ptp, got, want, why);
        exit (1);
    }
}

int main (void)
{
    /* Unsupported or inapplicable property: not a bad parameter. This is the
     * code seen live from the K-1 II for a descriptor the body declines. */
    expect (PTP_RC_DevicePropNotSupported, GP_ERROR_NOT_SUPPORTED,
        "declined descriptor must read as unsupported");
    expect (PTP_RC_InvalidObjectFormatCode, GP_ERROR_NOT_SUPPORTED,
        "format the body has no notion of");
    expect (PTP_RC_SpecificationByFormatUnsupported, GP_ERROR_NOT_SUPPORTED,
        "unsupported specification by format");
    expect (PTP_RC_SpecificationOfDestinationUnsupported, GP_ERROR_NOT_SUPPORTED,
        "unsupported destination");
    expect (PTP_RC_InvalidCodeFormat, GP_ERROR_NOT_SUPPORTED,
        "unsupported code format");
    expect (PTP_RC_NoStreamEnabled, GP_ERROR_NOT_SUPPORTED,
        "no stream enabled");

    /* The body rejected a value: the client asked for something concrete that
     * the camera refused, which is a parameter problem, not an I/O one. */
    expect (PTP_RC_InvalidDevicePropValue, GP_ERROR_BAD_PARAMETERS,
        "rejected property value");
    expect (PTP_RC_InvalidDevicePropFormat, GP_ERROR_BAD_PARAMETERS,
        "rejected property format");
    expect (PTP_RC_InvalidParameter, GP_ERROR_BAD_PARAMETERS,
        "rejected parameter");
    expect (PTP_RC_InvalidObjectHandle, GP_ERROR_BAD_PARAMETERS,
        "stale object handle");
    expect (PTP_RC_InvalidStorageId, GP_ERROR_BAD_PARAMETERS,
        "unknown storage");
    expect (PTP_RC_InvalidParentObject, GP_ERROR_BAD_PARAMETERS,
        "bad parent");
    expect (PTP_RC_InvalidEnumHandle, GP_ERROR_BAD_PARAMETERS,
        "bad enum handle");
    expect (PTP_RC_InvalidDataSet, GP_ERROR_BAD_PARAMETERS, "bad data set");
    expect (PTP_RC_NoValidObjectInfo, GP_ERROR_BAD_PARAMETERS,
        "no object info");

    /* Card conditions are distinguishable from one another. */
    expect (PTP_RC_StoreFull, GP_ERROR_FIXED_LIMIT_EXCEEDED, "card full");
    expect (PTP_RC_StoreNotAvailable, GP_ERROR_IO, "store unavailable");
    expect (PTP_RC_StoreReadOnly, GP_ERROR_IO, "store read-only");
    expect (PTP_RC_AccessDenied, GP_ERROR_IO, "access denied");

    /* Previously mapped behaviour must not change: callers of these paths rely
     * on it, including the Pentax session-recovery logic. */
    expect (PTP_RC_OK, GP_OK, "success");
    expect (PTP_RC_ParameterNotSupported, GP_ERROR_BAD_PARAMETERS, "unchanged");
    expect (PTP_RC_OperationNotSupported, GP_ERROR_NOT_SUPPORTED, "unchanged");
    expect (PTP_RC_DeviceBusy, GP_ERROR_CAMERA_BUSY, "unchanged");
    expect (PTP_ERROR_TIMEOUT, GP_ERROR_TIMEOUT, "unchanged");
    expect (PTP_ERROR_CANCEL, GP_ERROR_CANCEL, "unchanged");
    expect (PTP_ERROR_IO, GP_ERROR_IO, "unchanged");
    expect (PTP_RC_CaptureAlreadyTerminated, GP_OK,
        "kept: session logic depends on this");

    /* Protocol-level codes deliberately keep the generic result; changing them
     * would alter session recovery, which is out of scope here. */
    expect (PTP_RC_SessionNotOpen, GP_ERROR, "left generic on purpose");
    expect (PTP_RC_SessionAlreadyOpened, GP_ERROR, "left generic on purpose");
    expect (PTP_RC_InvalidTransactionID, GP_ERROR, "left generic on purpose");

    /* A genuinely unknown code still reports generically. */
    expect (0x2fff, GP_ERROR, "unknown code stays generic");

    printf ("ok\n");
    return 0;
}
