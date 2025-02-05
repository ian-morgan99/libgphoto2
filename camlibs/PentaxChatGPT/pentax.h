#ifndef _PENTAX_CAMERA_H_
#define _PENTAX_CAMERA_H_

#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-ptp.h>
#include <gphoto2/gphoto2-context.h>
#include <stdint.h>

/* Define Pentax Vendor and Model IDs */
#define PENTAX_VENDOR_ID 0x0A17

/* Model Numbers */
#define MODELNO_645D   77320
#define MODELNO_K_3    77760
#define MODELNO_K_3III 78420
#define MODELNO_645Z   77840
#define MODELNO_FF     77970
#define MODELNO_FFII   78400
#define MODELNO_K70    78370
#define MODELNO_KP     78380
#define MODELNO_G900SE 78460
#define MODELNO_GRIII  78350

/* Model Names */
#define MODEL_645D   "645D"
#define MODEL_K_3    "PENTAX K-3"
#define MODEL_K_3III "PENTAX K-3 Mark III"
#define MODEL_645Z   "Pentax 645Z"
#define MODEL_FF     "PENTAX K-1"
#define MODEL_FFII   "PENTAX K-1 Mark II"
#define MODEL_K70    "PENTAX K-70"
#define MODEL_KP     "Pentax KP"
#define MODEL_G900SE "RICOH G900SE"
#define MODEL_GRIII  "RICOH GR III"

/* PTP Vendor Codes for Pentax-specific operations */
#define PTP_VENDOR_PENTAX_GET_LIVE_VIEW_FRAME_DATA    0x9064
#define PTP_VENDOR_PENTAX_ENABLE_ASTROTRACER          0x9090
#define PTP_VENDOR_PENTAX_SET_ASTROTRACER_TIME_LIMIT    0x9091
#define PTP_VENDOR_PENTAX_SET_ASTROTRACER_MODE          0x9092
#define PTP_VENDOR_PENTAX_BULB_START                    0x1005
#define PTP_VENDOR_PENTAX_BULB_STOP                     0x1006
#define PTP_VENDOR_PENTAX_SET_BULB_TIMER                0x1007
#define PTP_VENDOR_PENTAX_SET_EXPOSURE_MODE             0x1008
#define PTP_VENDOR_PENTAX_AF_TRIGGER                    0x1009
#define PTP_VENDOR_PENTAX_GET_AF_STATUS                 0x100A
#define PTP_VENDOR_PENTAX_FOCUS_STEP                    0x100B
#define PTP_VENDOR_PENTAX_SET_FOCUS_POSITION            0x100C
#define PTP_VENDOR_PENTAX_SET_CAPTURE_MODE              0x100D
#define PTP_VENDOR_PENTAX_SET_INTERVAL_MODE             0x100E
#define PTP_VENDOR_PENTAX_GET_STORAGE_INFO              0x100F
#define PTP_VENDOR_PENTAX_GET_OBJECT                    0x1010

/* Additional Vendor-Specific PTP Commands */
#define PTP_VENDOR_PENTAX_SET_LIVE_VIEW        0x9064
#define PTP_VENDOR_PENTAX_STOP_LIVE_VIEW       0x9065
#define PTP_VENDOR_PENTAX_GET_LIVE_VIEW_FRAME  0x9024
#define PTP_VENDOR_PENTAX_SET_APERTURE         0x9066
#define PTP_VENDOR_PENTAX_SET_SHUTTER_SPEED    0x9067
#define PTP_VENDOR_PENTAX_SET_ISO              0x9068
#define PTP_VENDOR_PENTAX_AF_TRIGGER           0x9069
#define PTP_VENDOR_PENTAX_GET_AF_STATUS        0x9070
#define PTP_VENDOR_PENTAX_ENABLE_ASTROTRACER   0x9090
#define PTP_VENDOR_PENTAX_SET_ASTROTRACER_TIME_LIMIT 0x9091
#define PTP_VENDOR_PENTAX_SET_ASTROTRACER_MODE      0x9092
#define PTP_VENDOR_PENTAX_ADJUST_COMPOSITION        0x9093
#define PTP_VENDOR_PENTAX_SET_CAPTURE_MODE          0x906E
#define PTP_VENDOR_PENTAX_SET_INTERVAL_MODE         0x908F

// Astrotracer Exposure Mode
#define EXP_MODE_ASTROTRACER 20

// Response Codes
#define MTP_RC_Undefined 8192   // 0x2000
#define MTP_RC_Ok        8193   // 0x2001
#define MTP_RC_DeviceBusy 8217

// Capture Modes
#define CAPTURE_MODE_SINGLE_SHOT 0x01
#define CAPTURE_MODE_CONTINUOUS  0x02
#define CAPTURE_MODE_BULB        0x03
#define CAPTURE_MODE_BRACKETING  0x04
#define CAPTURE_MODE_INTERVAL    0x05

// Camera Capabilities (example values)
#define USB_VENDOR_ID_PENTAX       0x04BD
#define USB_PRODUCT_ID_PENTAX_K1II 0x503A
#define USB_PRODUCT_ID_PENTAX_K3III 0x5049
#define USB_PRODUCT_ID_PENTAX_K01  0x5028

// File Transfer and Storage Commands
#define PTP_VENDOR_PENTAX_FORMAT_STORE  0x4111

/* Low-Level PTP Commands (for sensor cleaning, WB, etc.) */
#define PTP_VENDOR_PENTAX_OC_ExecuteDustReduction       0x2412  /* Example: 36898 */
#define PTP_VENDOR_PENTAX_OC_MeasuringWhiteBalanceGain  0x2411  /* Example: 36897 */
#define PTP_VENDOR_PENTAX_OC_FocusControl               0x2416  /* Example: 36886 */
#define PTP_VENDOR_PENTAX_OC_MovieMode                  0xD005  /* Example: 53305 */
#define PTP_VENDOR_PENTAX_OC_InitiatePentaxCapture      0x2415  /* Example: 36881 */
#define PTP_VENDOR_PENTAX_OC_GetLiveViewFrameData       0x9064  /* Use same as live view command */

/* Function Prototypes */

// Device Identification and Initialization
int _camera_id(CameraText *id);
int _camera_abilities(CameraAbilitiesList *list);
int camera_init(Camera *camera, GPContext *context);
int pentax_exit(Camera *camera, GPContext *context);

// Image Capture
int pentax_capture_image(Camera *camera, GPContext *context);

// Camera Configuration
int pentax_get_config(Camera *camera, CameraWidget **window, GPContext *context);
int pentax_set_config(Camera *camera, CameraWidget *window, GPContext *context);

// Extended (PTP-based) Functions
int get_high_res_live_view(Camera *camera, CameraFile *file, GPContext *context);
int stop_live_view(Camera *camera, GPContext *context);
int enable_astrotracer(Camera *camera, GPContext *context);
int disable_astrotracer(Camera *camera, GPContext *context);
int set_astrotracer_mode(Camera *camera, GPContext *context, uint32_t mode);
int set_astrotracer_time_limit(Camera *camera, GPContext *context, uint32_t time_limit);
int start_bulb_exposure(Camera *camera, GPContext *context);
int stop_bulb_exposure(Camera *camera, GPContext *context);
int set_bulb_timer(Camera *camera, GPContext *context, uint32_t time_seconds);
int set_exposure_mode(Camera *camera, GPContext *context, uint32_t mode);
int adjust_focus(Camera *camera, GPContext *context, int step);
int set_focus_position(Camera *camera, GPContext *context, uint32_t position);
int trigger_autofocus(Camera *camera, GPContext *context);
int transfer_file(Camera *camera, GPContext *context, uint32_t object_handle, CameraFile *file);
int get_storage_info(Camera *camera, GPContext *context);
int set_multi_exposure_mode(Camera *camera, GPContext *context, uint32_t mode);
int set_bracketing_mode(Camera *camera, GPContext *context, uint32_t mode);
int set_interval_shooting_mode(Camera *camera, GPContext *context, uint32_t interval_seconds);

// Low-Level PTP Functions
int pentax_clean_sensor(PTPParams* params);
int pentax_measure_wb(PTPParams* params);
int pentax_focus_control(PTPParams* params, int direction);
int pentax_set_movie_mode(PTPParams* params, int enable);
int pentax_capture_ptp(PTPParams* params);
int pentax_get_liveview(PTPParams* params, uint8_t* buffer, int bufsize);

#endif // _PENTAX_CAMERA_H_
