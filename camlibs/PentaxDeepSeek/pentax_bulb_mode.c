#include "pentax_enums.h"
#include "pentax_ptp_commands.h"
#include <gphoto2/gphoto2.h>

int start_bulb_mode(GPhoto2Camera *camera, GPhoto2Context *context) {
    GPhoto2PtpContainer *request = NULL;
    GPhoto2PtpContainer *response = NULL;
    int ret = 0;

    request = gp_ptp_container_new();
    if (!request)
        return GP_ERROR_NO_MEMORY;

    gp_ptp_set_command(request, PTP_VENDOR_PENTAX_BULB_MODE_START);

    ret = gp_ptp_transfer(camera, request, &response, context);
    if (ret != GP_OK) {
        g_object_unref(response);
        g_object_unref(request);
        return ret;
    }

    g_object_unref(request);
    g_object_unref(response);
    return ret;
}

int stop_bulb_mode(GPhoto2Camera *camera, GPhoto2Context *context) {
    GPhoto2PtpContainer *request = NULL;
    GPhoto2PtpContainer *response = NULL;
    int ret = 0;

    request = gp_ptp_container_new();
    if (!request)
        return GP_ERROR_NO_MEMORY;

    gp_ptp_set_command(request, PTP_VENDOR_PENTAX_BULB_MODE_STOP);

    ret = gp_ptp_transfer(camera, request, &response, context);
    if (ret != GP_OK) {
        g_object_unref(response);
        g_object_unref(request);
        return ret;
    }

    g_object_unref(request);
    g_object_unref(response);
    return ret;
}
