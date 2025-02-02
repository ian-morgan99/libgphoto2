#include "pentax.h"

int start_high_res_live_view(Camera *camera, GPContext *context);
int stop_high_res_live_view(Camera *camera, GPContext *context);
int get_live_view_frame_data(Camera *camera, CameraFile *file, GPContext *context);


int get_live_view_frame_data(Camera *camera, CameraFile *file, GPContext *context) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_GET_LIVE_VIEW_FRAME_DATA);
    if (!request) return GP_ERROR_NO_MEMORY;

    int ret = gp_ptp_transfer(camera, request, &response, context);
    gp_ptp_container_free(request);

    if (ret >= GP_OK && response) {
        ushort *frame_data = malloc(response->Payload.Length);
        memcpy(frame_data, response->Payload.Data, response->Payload.Length);
        gp_ptp_container_free(response);

        return frame_data;
    }

    gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to get live view frame data.");
    return NULL;
}
