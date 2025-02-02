#ifndef _PENTAX_CAMERA_H_
#define _PENTAX_CAMERA_H_

#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-context.h>
#include <stdint.h>

// Vendor-Specific PTP Commands
#define PTP_VENDOR_PENTAX_SET_LIVE_VIEW        0x9064
#define PTP_VENDOR_PENTAX_STOP_LIVE_VIEW       0x9065
#define PTP_VENDOR_PENTAX_GET_LIVE_VIEW_FRAME  0x9024
#define PTP_VENDOR_PENTAX_SET_APERTURE         0x9066
#define PTP_VENDOR_PENTAX_SET_SHUTTER_SPEED    0x9067
#define PTP_VENDOR_PENTAX_SET_ISO              0x9068
#define PTP_VENDOR_PENTAX_AF_TRIGGER           0x9069
#define PTP_VENDOR_PENTAX_GET_AF_STATUS        0x9070
#define PTP_VENDOR_PENTAX_ENABLE_ASTROTRACER        0x9090
#define PTP_VENDOR_PENTAX_SET_ASTROTRACER_TIME_LIMIT 0x9091
#define PTP_VENDOR_PENTAX_SET_ASTROTRACER_MODE      0x9092
#define PTP_VENDOR_PENTAX_ADJUST_COMPOSITION        0x9093
#define PTP_VENDOR_PENTAX_SET_CAPTURE_MODE          0x906E
#define PTP_VENDOR_PENTAX_SET_INTERVAL_MODE         0x908F
// High-Resolution Live View Commands
#define PTP_VENDOR_PENTAX_GET_LIVE_VIEW_FRAME_DATA  0x9064



// Astrotracer Exposure Mode
#define EXP_MODE_ASTROTRACER                        20


// Response Codes
#define MTP_RC_Undefined                       8192   // 0x2000
#define MTP_RC_Ok                              8193   // 0x2001
#define MTP_RC_DeviceBusy                      8217

// Capture Modes
#define CAPTURE_MODE_SINGLE_SHOT                   0x01
#define CAPTURE_MODE_CONTINUOUS                    0x02
#define CAPTURE_MODE_BULB                          0x03
#define CAPTURE_MODE_BRACKETING                    0x04
#define CAPTURE_MODE_INTERVAL                      0x05

// Camera Capabilities
#define USB_VENDOR_ID_PENTAX                   0x04BD
#define USB_PRODUCT_ID_PENTAX_K1II             0x503A
#define USB_PRODUCT_ID_PENTAX_K3III            0x5049
#define USB_PRODUCT_ID_PENTAX_K01              0x5028
// Astrotracer Mode
#define EXP_MODE_ASTROTRACER                        20
// File Transfer and Storage Commands
#define PTP_VENDOR_PENTAX_GET_STORAGE_INFO         0x4101
#define PTP_VENDOR_PENTAX_FORMAT_STORE            0x4111
#define PTP_VENDOR_PENTAX_GET_OBJECT              0x4105

// Function Prototypes

// Device Identification and Initialization
int _camera_id(CameraText *id);
int _camera_abilities(CameraAbilitiesList *list);
int camera_init(Camera *camera, GPContext *context);

int trigger_autofocus(Camera *camera, GPContext *context);
int adjust_focus(Camera *camera, GPContext *context, int step);
int set_focus_position(Camera *camera, GPContext *context, uint32_t position);


// Image Capture
int capture_image(Camera *camera, GPContext *context);

// Live View
int get_high_res_live_view(Camera *camera, CameraFile *file, GPContext *context);

// Astrotracer
int enable_astrotracer(Camera *camera, GPContext *context);
int disable_astrotracer(Camera *camera, GPContext *context);
int set_astrotracer_mode(Camera *camera, GPContext *context, uint32_t mode);
int set_astrotracer_time_limit(Camera *camera, GPContext *context, uint32_t time_limit);

// Bulb
int set_exposure_mode(Camera *camera, GPContext *context, uint32_t mode);
int start_bulb_exposure(Camera *camera, GPContext *context);
int stop_bulb_exposure(Camera *camera, GPContext *context);


// File Transfer and Storage
int get_storage_info(Camera *camera, GPContext *context);
int transfer_file(Camera *camera, GPContext *context, uint32_t object_handle, CameraFile *file);

// Extended Capture Modes
int set_multi_exposure_mode(Camera *camera, GPContext *context, uint32_t mode);
int set_bracketing_mode(Camera *camera, GPContext *context, uint32_t mode);
int set_interval_shooting_mode(Camera *camera, GPContext *context, uint32_t interval_seconds);

int camera_init(Camera *camera, GPContext *context) {
    camera->functions->id = _camera_id;
    camera->functions->abilities = _camera_abilities;
    camera->functions->capture = capture_image;
    camera->functions->set_exposure_mode = set_exposure_mode;
    camera->functions->start_bulb_exposure = start_bulb_exposure;
    camera->functions->stop_bulb_exposure = stop_bulb_exposure;
    camera->functions->set_bulb_timer = set_bulb_timer;
    camera->functions->start_live_view = get_high_res_live_view;
    camera->functions->stop_live_view = stop_live_view;
    camera->functions->trigger_autofocus = trigger_autofocus;
    camera->functions->adjust_focus = adjust_focus;
    camera->functions->set_focus_position = set_focus_position;
    camera->functions->enable_astrotracer = enable_astrotracer;
    camera->functions->disable_astrotracer = disable_astrotracer;
    camera->functions->set_astrotracer_mode = set_astrotracer_mode;
    camera->functions->set_astrotracer_time_limit = set_astrotracer_time_limit;
    camera->functions->get_storage_info = get_storage_info;
    camera->functions->transfer_file = transfer_file;
    camera->functions->set_multi_exposure_mode = set_multi_exposure_mode;
    camera->functions->set_bracketing_mode = set_bracketing_mode;
    camera->functions->set_interval_shooting_mode = set_interval_shooting_mode;
    return GP_OK;
}


#endif // _PENTAX_CAMERA_H_