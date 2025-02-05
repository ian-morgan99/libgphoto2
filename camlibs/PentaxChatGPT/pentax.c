/* pentax.c - Pentax driver for libgphoto2 with extended functionalities.
 *
 * Supported Models:
 *   645D, PENTAX K-3, PENTAX K-3 Mark III, Pentax 645Z, 
 *   PENTAX K-1, PENTAX K-1 Mark II, PENTAX K-70, Pentax KP,
 *   RICOH G900SE, and RICOH GR III.
 *
 * Features include:
 *   - Standard capture (with Bulb mode support)
 *   - High-Resolution Live View
 *   - Extended capture modes (multi-exposure, bracketing, interval shooting)
 *   - Advanced Astrotracer (including time-limit support)
 *   - Enhanced autofocus, focus control, and file transfer
 *   - Low-level PTP commands (sensor cleaning, white balance, movie mode, etc.)
 *
 * Note: Ensure that "pentax_camera.h" is included in your project.
 */

#include "pentax_camera.h"
#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-port-log.h>
#include <stdlib.h>
#include <string.h>

/* ---------------- Vendor and Model Definitions ---------------- */
#define PENTAX_VENDOR_ID 0x0A17

// Model Numbers
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

// Model Names
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

/* ---------------- Centralized Error Handling ---------------- */
const char *map_mtp_response_code(ushort code) {
    switch (code) {
        case 8192: return "Undefined error.";
        case 8193: return "Operation successful.";
        case 8197: return "Operation not supported.";
        case 8217: return "Device is busy. Retry later.";
        case 8221: return "Invalid parameter.";
        case 40961: return "Pentax capture general error.";
        case 40962: return "Pentax capture release locked.";
        case 40963: return "Flash charging. Operation delayed.";
        case 40964: return "Autofocus failed.";
        case 40972: return "Focus control failed.";
        default:   return "Unknown error code.";
    }
}

int handle_mtp_response(ushort response_code) {
    const char *error_message = map_mtp_response_code(response_code);
    if (response_code != MTP_RC_Ok) {  // 8193 indicates "Ok"
        gp_log(GP_LOG_ERROR, "Pentax Driver", "Error: %s (Code: %d)", error_message, response_code);
        return GP_ERROR;
    }
    return GP_OK;
}

/* ---------------- Supported Models Structure ---------------- */
static const struct {
    const char *model;
    uint16_t usb_vendor;
    uint16_t usb_product;
} pentax_models[] = {
    {MODEL_645D,   PENTAX_VENDOR_ID, MODELNO_645D},
    {MODEL_K_3,    PENTAX_VENDOR_ID, MODELNO_K_3},
    {MODEL_K_3III, PENTAX_VENDOR_ID, MODELNO_K_3III},
    {MODEL_645Z,   PENTAX_VENDOR_ID, MODELNO_645Z},
    {MODEL_FF,     PENTAX_VENDOR_ID, MODELNO_FF},
    {MODEL_FFII,   PENTAX_VENDOR_ID, MODELNO_FFII},
    {MODEL_K70,    PENTAX_VENDOR_ID, MODELNO_K70},
    {MODEL_KP,     PENTAX_VENDOR_ID, MODELNO_KP},
    {MODEL_G900SE, PENTAX_VENDOR_ID, MODELNO_G900SE},
    {MODEL_GRIII,  PENTAX_VENDOR_ID, MODELNO_GRIII},
};

/* ---------------- Camera Operations Registration ---------------- */
static CameraOperations pentax_ops = {
    .init       = pentax_init,
    .exit       = pentax_exit,
    .capture    = pentax_capture_image,
    .get_config = pentax_get_config,
    .set_config = pentax_set_config,
    /* Additional operations can be added here */
};

int camera_abilities(CameraAbilitiesList *list) {
    CameraAbilities abilities;
    int i;
    for (i = 0; i < (int)(sizeof(pentax_models) / sizeof(pentax_models[0])); i++) {
        memset(&abilities, 0, sizeof(CameraAbilities));
        strcpy(abilities.model, pentax_models[i].model);
        abilities.status = GP_DRIVER_STATUS_EXPERIMENTAL;
        abilities.port = GP_PORT_USB;
        abilities.usb_vendor = pentax_models[i].usb_vendor;
        abilities.usb_product = pentax_models[i].usb_product;
        abilities.operations = GP_OPERATION_CAPTURE_IMAGE | GP_OPERATION_CONFIG;
        abilities.file_operations = GP_FILE_OPERATION_DELETE;
        abilities.folder_operations = GP_FOLDER_OPERATION_NONE;
        gp_abilities_list_append(list, abilities);
    }
    return GP_OK;
}

/* ---------------- High-Level Driver Functions ---------------- */

/* Device Identification */
static int _camera_id(CameraText *id) {
    strncpy(id->text, "Pentax Driver", sizeof(id->text) - 1);
    id->text[sizeof(id->text) - 1] = '\0';
    return GP_OK;
}

/* Internal Abilities for Registration */
static int _camera_abilities(CameraAbilitiesList *list) {
    CameraAbilities a;
    memset(&a, 0, sizeof(a));
    strcpy(a.model, "Pentax-PentaxPlugin");
    a.status = GP_DRIVER_STATUS_EXPERIMENTAL;
    a.port = GP_PORT_USB;
    a.operations = GP_OPERATION_CAPTURE_IMAGE | GP_OPERATION_CONFIG;
    a.file_operations = GP_FILE_OPERATION_NONE;
    a.folder_operations = GP_FOLDER_OPERATION_NONE;
    a.usb_vendor = PENTAX_VENDOR_ID;
    a.usb_product = MODELNO_FF;  /* Default product; adjust as needed */
    gp_abilities_list_append(list, a);
    return GP_OK;
}

/* Initialize the camera (called when the device is opened) */
int pentax_init(Camera *camera, GPContext *context) {
    camera->functions->id = _camera_id;
    camera->functions->abilities = _camera_abilities;
    camera->functions->capture = pentax_capture_image;
    camera->functions->get_config = pentax_get_config;
    camera->functions->set_config = pentax_set_config;
    camera->functions->start_live_view = get_high_res_live_view;
    camera->functions->stop_live_view = stop_live_view;
    camera->functions->trigger_autofocus = trigger_autofocus;
    camera->functions->set_exposure_mode = set_exposure_mode;
    camera->functions->start_bulb_exposure = start_bulb_exposure;
    camera->functions->stop_bulb_exposure = stop_bulb_exposure;
    camera->functions->set_bulb_timer = set_bulb_timer;
    camera->functions->enable_astrotracer = enable_astrotracer;
    camera->functions->disable_astrotracer = disable_astrotracer;
    camera->functions->set_astrotracer_mode = set_astrotracer_mode;
    camera->functions->set_astrotracer_time_limit = set_astrotracer_time_limit;
    camera->functions->adjust_focus = adjust_focus;
    camera->functions->set_focus_position = set_focus_position;
    camera->functions->get_storage_info = get_storage_info;
    camera->functions->transfer_file = transfer_file;
    camera->functions->set_multi_exposure_mode = set_multi_exposure_mode;
    camera->functions->set_bracketing_mode = set_bracketing_mode;
    camera->functions->set_interval_shooting_mode = set_interval_shooting_mode;
    return GP_OK;
}

/* Exit/Close the camera */
int pentax_exit(Camera *camera, GPContext *context) {
    /* Cleanup resources if necessary */
    return GP_OK;
}

/* Get the camera configuration */
int pentax_get_config(Camera *camera, CameraWidget **window, GPContext *context) {
    return gp_widget_new(GP_WIDGET_WINDOW, "Pentax Configuration", window);
}

/* Set the camera configuration */
int pentax_set_config(Camera *camera, CameraWidget *window, GPContext *context) {
    return gp_camera_set_config(camera, window, context);
}

/* Standard Capture Function (handles Bulb mode if applicable) */
int pentax_capture_image(Camera *camera, GPContext *context) {
    CameraWidget *widget = NULL, *shutter_widget = NULL;
    const char *shutter_value;
    int ret = gp_camera_get_config(camera, &widget, context);
    if (ret < GP_OK) {
        gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to get camera configuration.");
        return ret;
    }
    ret = gp_widget_get_child_by_name(widget, "shutterspeed", &shutter_widget);
    if (ret < GP_OK) {
        gp_log(GP_LOG_ERROR, "Pentax Driver", "Shutter speed widget not found.");
        gp_widget_free(widget);
        return ret;
    }
    ret = gp_widget_get_value(shutter_widget, &shutter_value);
    if (ret < GP_OK) {
        gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to get shutter speed value.");
        gp_widget_free(widget);
        return ret;
    }
    /* Check for Bulb mode ("B") */
    if (strcmp(shutter_value, "B") == 0) {
        gp_log(GP_LOG_INFO, "Pentax Driver", "Bulb mode detected. Starting Bulb exposure.");
        ret = start_bulb_exposure(camera, context);
        if (ret < GP_OK) {
            gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to start Bulb exposure.");
            gp_widget_free(widget);
            return ret;
        }
        ret = gp_camera_wait_for_event(camera, GP_EVENT_CAPTURE_COMPLETE, context);
        if (ret < GP_OK) {
            gp_log(GP_LOG_ERROR, "Pentax Driver", "Capture event failed, stopping Bulb exposure.");
            stop_bulb_exposure(camera, context);
            gp_widget_free(widget);
            return ret;
        }
        ret = stop_bulb_exposure(camera, context);
        if (ret < GP_OK) {
            gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to stop Bulb exposure.");
            gp_widget_free(widget);
            return ret;
        }
        gp_log(GP_LOG_INFO, "Pentax Driver", "Bulb exposure completed.");
        gp_widget_free(widget);
        return GP_OK;
    }
    gp_widget_free(widget);
    /* For non-Bulb mode, perform a standard capture */
    return gp_camera_capture(camera, GP_CAPTURE_IMAGE, NULL, context);
}

/* ---------------- Extended Function Implementations ---------------- */

/* High-Resolution Live View */
int get_high_res_live_view(Camera *camera, CameraFile *file, GPContext *context) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_GET_LIVE_VIEW_FRAME_DATA);
    if (!request) return GP_ERROR_NO_MEMORY;
    PtpContainer *response = NULL;
    int ret = gp_ptp_transfer(camera, request, &response, context);
    gp_ptp_container_free(request);
    if (ret >= GP_OK && response) {
        ushort response_code = gp_ptp_get_response_code(response);
        int err = handle_mtp_response(response_code);
        if (err != GP_OK) {
            gp_ptp_container_free(response);
            return err;
        }
        gp_file_set_data_and_size(file, (char *)response->Payload.Data, response->Payload.Length);
        gp_ptp_container_free(response);
    }
    return ret;
}

/* Stop Live View */
int stop_live_view(Camera *camera, GPContext *context) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_STOP_LIVE_VIEW);
    if (!request) return GP_ERROR_NO_MEMORY;
    PtpContainer *response = NULL;
    int ret = gp_ptp_transfer(camera, request, &response, context);
    gp_ptp_container_free(request);
    if (ret >= GP_OK && response) {
        ushort response_code = gp_ptp_get_response_code(response);
        ret = handle_mtp_response(response_code);
        gp_ptp_container_free(response);
    }
    return ret;
}

/* Astrotracer Functions */
int enable_astrotracer(Camera *camera, GPContext *context) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_ENABLE_ASTROTRACER);
    if (!request) return GP_ERROR_NO_MEMORY;
    gp_ptp_container_add_uint32(request, 1); // Enable Astrotracer
    PtpContainer *response = NULL;
    int ret = gp_ptp_transfer(camera, request, &response, context);
    gp_ptp_container_free(request);
    if (ret >= GP_OK && response) {
        ushort response_code = gp_ptp_get_response_code(response);
        ret = handle_mtp_response(response_code);
        gp_ptp_container_free(response);
        return ret;
    }
    if (response) gp_ptp_container_free(response);
    return ret;
}

int disable_astrotracer(Camera *camera, GPContext *context) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_ENABLE_ASTROTRACER);
    if (!request) return GP_ERROR_NO_MEMORY;
    gp_ptp_container_add_uint32(request, 0); // Disable Astrotracer
    PtpContainer *response = NULL;
    int ret = gp_ptp_transfer(camera, request, &response, context);
    gp_ptp_container_free(request);
    if (ret >= GP_OK && response) {
        ushort response_code = gp_ptp_get_response_code(response);
        ret = handle_mtp_response(response_code);
        gp_ptp_container_free(response);
        return ret;
    }
    if (response) gp_ptp_container_free(response);
    return ret;
}

int set_astrotracer_mode(Camera *camera, GPContext *context, uint32_t mode) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_SET_ASTROTRACER_MODE);
    if (!request) return GP_ERROR_NO_MEMORY;
    gp_ptp_container_add_uint32(request, mode);
    PtpContainer *response = NULL;
    int ret = gp_ptp_transfer(camera, request, &response, context);
    gp_ptp_container_free(request);
    if (ret >= GP_OK && response) {
        ushort response_code = gp_ptp_get_response_code(response);
        ret = handle_mtp_response(response_code);
        gp_ptp_container_free(response);
        return ret;
    }
    if (response) gp_ptp_container_free(response);
    gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to set Astrotracer mode.");
    return ret;
}

int set_astrotracer_time_limit(Camera *camera, GPContext *context, uint32_t time_limit) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_SET_ASTROTRACER_TIME_LIMIT);
    if (!request) return GP_ERROR_NO_MEMORY;
    gp_ptp_container_add_uint32(request, time_limit);
    PtpContainer *response = NULL;
    int ret = gp_ptp_transfer(camera, request, &response, context);
    gp_ptp_container_free(request);
    if (ret >= GP_OK && response) {
        ushort response_code = gp_ptp_get_response_code(response);
        ret = handle_mtp_response(response_code);
        gp_ptp_container_free(response);
        return ret;
    }
    if (response) gp_ptp_container_free(response);
    gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to set Astrotracer time limit.");
    return ret;
}

/* Bulb Mode Functions */
int start_bulb_exposure(Camera *camera, GPContext *context) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_BULB_START);
    if (!request) return GP_ERROR_NO_MEMORY;
    PtpContainer *response = NULL;
    int ret = gp_ptp_transfer(camera, request, &response, context);
    gp_ptp_container_free(request);
    if (ret >= GP_OK && response) {
        ushort response_code = gp_ptp_get_response_code(response);
        ret = handle_mtp_response(response_code);
        gp_ptp_container_free(response);
        return ret;
    }
    if (response) gp_ptp_container_free(response);
    gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to start Bulb exposure.");
    return ret;
}

int stop_bulb_exposure(Camera *camera, GPContext *context) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_BULB_STOP);
    if (!request) return GP_ERROR_NO_MEMORY;
    PtpContainer *response = NULL;
    int ret = gp_ptp_transfer(camera, request, &response, context);
    gp_ptp_container_free(request);
    if (ret >= GP_OK && response) {
        ushort response_code = gp_ptp_get_response_code(response);
        ret = handle_mtp_response(response_code);
        gp_ptp_container_free(response);
        return ret;
    }
    if (response) gp_ptp_container_free(response);
    gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to stop Bulb exposure.");
    return ret;
}

int set_bulb_timer(Camera *camera, GPContext *context, uint32_t time_seconds) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_SET_BULB_TIMER);
    if (!request) return GP_ERROR_NO_MEMORY;
    gp_ptp_container_add_uint32(request, time_seconds);
    PtpContainer *response = NULL;
    int ret = gp_ptp_transfer(camera, request, &response, context);
    gp_ptp_container_free(request);
    if (ret >= GP_OK && response) {
        ushort response_code = gp_ptp_get_response_code(response);
        ret = handle_mtp_response(response_code);
        gp_ptp_container_free(response);
        return ret;
    }
    if (response) gp_ptp_container_free(response);
    gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to set Bulb timer.");
    return ret;
}

/* Exposure Mode */
int set_exposure_mode(Camera *camera, GPContext *context, uint32_t mode) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_SET_EXPOSURE_MODE);
    if (!request) return GP_ERROR_NO_MEMORY;
    gp_ptp_container_add_uint32(request, mode);
    PtpContainer *response = NULL;
    int ret = gp_ptp_transfer(camera, request, &response, context);
    gp_ptp_container_free(request);
    if (ret >= GP_OK && response) {
        ushort response_code = gp_ptp_get_response_code(response);
        ret = handle_mtp_response(response_code);
        gp_ptp_container_free(response);
        return ret;
    }
    if (response) gp_ptp_container_free(response);
    gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to set exposure mode: %d", mode);
    return ret;
}

/* Focus Control */
int adjust_focus(Camera *camera, GPContext *context, int step) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_FOCUS_STEP);
    if (!request) return GP_ERROR_NO_MEMORY;
    gp_ptp_container_add_int32(request, step);
    PtpContainer *response = NULL;
    int ret = gp_ptp_transfer(camera, request, &response, context);
    gp_ptp_container_free(request);
    if (ret >= GP_OK && response) {
        ushort response_code = gp_ptp_get_response_code(response);
        ret = handle_mtp_response(response_code);
        gp_ptp_container_free(response);
        return ret;
    }
    if (response) gp_ptp_container_free(response);
    gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to adjust focus by step: %d", step);
    return ret;
}

int set_focus_position(Camera *camera, GPContext *context, uint32_t position) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_SET_FOCUS_POSITION);
    if (!request) return GP_ERROR_NO_MEMORY;
    gp_ptp_container_add_uint32(request, position);
    PtpContainer *response = NULL;
    int ret = gp_ptp_transfer(camera, request, &response, context);
    gp_ptp_container_free(request);
    if (ret >= GP_OK && response) {
        ushort response_code = gp_ptp_get_response_code(response);
        ret = handle_mtp_response(response_code);
        gp_ptp_container_free(response);
        return ret;
    }
    if (response) gp_ptp_container_free(response);
    gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to set focus position to: %d", position);
    return ret;
}

/* Autofocus Trigger */
int trigger_autofocus(Camera *camera, GPContext *context) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_AF_TRIGGER);
    if (!request) return GP_ERROR_NO_MEMORY;
    PtpContainer *response = NULL;
    int ret = gp_ptp_transfer(camera, request, &response, context);
    gp_ptp_container_free(request);
    if (ret >= GP_OK && response) {
        ushort response_code = gp_ptp_get_response_code(response);
        int error_check = handle_mtp_response(response_code);
        gp_ptp_container_free(response);
        if (error_check != GP_OK) {
            gp_log(GP_LOG_ERROR, "Pentax Driver", "Autofocus trigger failed.");
            return error_check;
        }
    } else {
        gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to send autofocus trigger command.");
        return ret;
    }
    return GP_OK;
}

/* File Transfer */
int transfer_file(Camera *camera, GPContext *context, uint32_t object_handle, CameraFile *file) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_GET_OBJECT);
    if (!request) return GP_ERROR_NO_MEMORY;
    gp_ptp_container_add_uint32(request, object_handle);
    PtpContainer *response = NULL;
    int ret = gp_ptp_transfer(camera, request, &response, context);
    gp_ptp_container_free(request);
    if (ret >= GP_OK && response) {
        ushort response_code = gp_ptp_get_response_code(response);
        int error_check = handle_mtp_response(response_code);
        if (error_check != GP_OK) {
            gp_log(GP_LOG_ERROR, "Pentax Driver", "File transfer failed. Object Handle: %d", object_handle);
            gp_ptp_container_free(response);
            return error_check;
        }
        gp_file_set_data_and_size(file, (char *)response->Payload.Data, response->Payload.Length);
        gp_ptp_container_free(response);
        return GP_OK;
    }
    if (response) gp_ptp_container_free(response);
    gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to transfer file. Object Handle: %d", object_handle);
    return ret;
}

/* Storage Info */
int get_storage_info(Camera *camera, GPContext *context) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_GET_STORAGE_INFO);
    if (!request) return GP_ERROR_NO_MEMORY;
    PtpContainer *response = NULL;
    int ret = gp_ptp_transfer(camera, request, &response, context);
    gp_ptp_container_free(request);
    if (ret >= GP_OK && response) {
        ushort response_code = gp_ptp_get_response_code(response);
        int error_check = handle_mtp_response(response_code);
        if (error_check != GP_OK) {
            gp_ptp_container_free(response);
            return error_check;
        }
        gp_ptp_container_free(response);
    }
    return ret;
}

/* Extended Capture Modes */
int set_multi_exposure_mode(Camera *camera, GPContext *context, uint32_t mode) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_SET_CAPTURE_MODE);
    if (!request) return GP_ERROR_NO_MEMORY;
    gp_ptp_container_add_uint32(request, mode);
    PtpContainer *response = NULL;
    int ret = gp_ptp_transfer(camera, request, &response, context);
    gp_ptp_container_free(request);
    if (ret >= GP_OK && response) {
        ushort response_code = gp_ptp_get_response_code(response);
        ret = handle_mtp_response(response_code);
        gp_ptp_container_free(response);
        return ret;
    }
    if (response) gp_ptp_container_free(response);
    return ret;
}

int set_bracketing_mode(Camera *camera, GPContext *context, uint32_t mode) {
    return set_multi_exposure_mode(camera, context, mode);
}

int set_interval_shooting_mode(Camera *camera, GPContext *context, uint32_t interval_seconds) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_SET_INTERVAL_MODE);
    if (!request) return GP_ERROR_NO_MEMORY;
    gp_ptp_container_add_uint32(request, interval_seconds);
    PtpContainer *response = NULL;
    int ret = gp_ptp_transfer(camera, request, &response, context);
    gp_ptp_container_free(request);
    if (ret >= GP_OK && response) {
        ushort response_code = gp_ptp_get_response_code(response);
        ret = handle_mtp_response(response_code);
        gp_ptp_container_free(response);
        return ret;
    }
    if (response) gp_ptp_container_free(response);
    return ret;
}

/* ---------------- Low-Level PTP Functions ---------------- */

/* Clean Sensor (Dust Reduction) */
int pentax_clean_sensor(PTPParams* params) {
    PTPContainer ptp;
    ptp.Code = PTP_VENDOR_PENTAX_OC_ExecuteDustReduction;
    ptp.Nparam = 0;
    int ret = ptp_transaction(params, &ptp, PTP_DP_NODATA, 0, NULL, NULL);
    if (ret != PTP_RC_OK) {
        GP_LOG_ERROR("Pentax Sensor Cleaning failed: %d", ret);
        return ret;
    }
    return GP_OK;
}

/* Measure White Balance */
int pentax_measure_wb(PTPParams* params) {
    PTPContainer ptp;
    ptp.Code = PTP_VENDOR_PENTAX_OC_MeasuringWhiteBalanceGain;
    ptp.Nparam = 0;
    int ret = ptp_transaction(params, &ptp, PTP_DP_NODATA, 0, NULL, NULL);
    if (ret != PTP_RC_OK) {
        GP_LOG_ERROR("Pentax White Balance measurement failed: %d", ret);
        return ret;
    }
    return GP_OK;
}

/* Focus Control */
int pentax_focus_control(PTPParams* params, int direction) {
    PTPContainer ptp;
    ptp.Code = PTP_VENDOR_PENTAX_OC_FocusControl;
    ptp.Nparam = 1;
    ptp.Param1 = direction;
    int ret = ptp_transaction(params, &ptp, PTP_DP_NODATA, 0, NULL, NULL);
    if (ret != PTP_RC_OK) {
        GP_LOG_ERROR("Pentax Focus Control failed: %d", ret);
        return ret;
    }
    return GP_OK;
}

/* Set Movie Mode */
int pentax_set_movie_mode(PTPParams* params, int enable) {
    PTPContainer ptp;
    ptp.Code = PTP_VENDOR_PENTAX_OC_MovieMode;
    ptp.Nparam = 1;
    ptp.Param1 = enable ? 1 : 0;
    int ret = ptp_transaction(params, &ptp, PTP_DP_NODATA, 0, NULL, NULL);
    if (ret != PTP_RC_OK) {
        GP_LOG_ERROR("Pentax Movie Mode failed: %d", ret);
        return ret;
    }
    return GP_OK;
}

/* Initiate Capture via PTP */
int pentax_capture_ptp(PTPParams* params) {
    PTPContainer ptp;
    ptp.Code = PTP_VENDOR_PENTAX_OC_InitiatePentaxCapture;
    ptp.Nparam = 0;
    int ret = ptp_transaction(params, &ptp, PTP_DP_NODATA, 0, NULL, NULL);
    if (ret != PTP_RC_OK) {
        GP_LOG_ERROR("Pentax Capture failed: %d", ret);
        return ret;
    }
    return GP_OK;
}

/* Get Live View Data via PTP */
int pentax_get_liveview(PTPParams* params, uint8_t* buffer, int bufsize) {
    PTPContainer ptp;
    ptp.Code = PTP_VENDOR_PENTAX_OC_GetLiveViewFrameData;
    ptp.Nparam = 0;
    int ret = ptp_transaction(params, &ptp, PTP_DP_GETDATA, 0, buffer, &bufsize);
    if (ret != PTP_RC_OK) {
        GP_LOG_ERROR("Pentax Live View failed: %d", ret);
        return ret;
    }
    return GP_OK;
}

/* ---------------- End of pentax.c ---------------- */
