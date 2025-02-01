// Refined pentax.c file with extended Astrotracer, High-Resolution Live View, File Transfer, and Capture Modes support
#include "pentax_camera.h"
#include <gphoto2/gphoto2-port-log.h>
#include <stdlib.h>
#include <string.h>


/**
 * Comprehensive camera driver for Pentax cameras with centralized error handling.
 * Fully integrates Astrotracer, Interval, File Transfer, High-Resolution Live View, and Capture Modes.
 */

// Centralized Error-Handling Logic for MTP Response Codes
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
        default: return "Unknown error code.";
    }
}

int handle_mtp_response(ushort response_code) {
    const char *error_message = map_mtp_response_code(response_code);
    if (response_code != 8193) { // Response is not "Ok"
        gp_log(GP_LOG_ERROR, "Pentax Driver", "Error: %s (Code: %d)", error_message, response_code);
        return GP_ERROR;
    }
    return GP_OK;
}



int camera_init(Camera *camera, GPContext *context) {
    // Assign the camera's identification function
    camera->functions->id = _camera_id;

    // Assign the camera's abilities function
    camera->functions->abilities = _camera_abilities;

    // Assign the capture function for taking images
    camera->functions->capture = capture_image;
    camera->functions->set_exposure_mode = set_exposure_mode;
    camera->functions->start_bulb_exposure = start_bulb_exposure;
    camera->functions->stop_bulb_exposure = stop_bulb_exposure;
    camera->functions->set_bulb_timer = set_bulb_timer;

    // Assign live view start and stop functions
    camera->functions->start_live_view = get_high_res_live_view;
    camera->functions->stop_live_view = stop_live_view;

    // Assign the autofocus trigger function
    camera->functions->trigger_autofocus = NULL; // Placeholder if autofocus trigger is implemented

    // Assign Astrotracer functions
    camera->functions->enable_astrotracer = enable_astrotracer;
    camera->functions->disable_astrotracer = disable_astrotracer;
    camera->functions->set_astrotracer_mode = set_astrotracer_mode;
    camera->functions->set_astrotracer_time_limit = set_astrotracer_time_limit;

    camera->functions->trigger_autofocus = trigger_autofocus;
    camera->functions->adjust_focus = adjust_focus;
    camera->functions->set_focus_position = set_focus_position;

    // Assign file transfer functions
    camera->functions->get_storage_info = get_storage_info;
    camera->functions->transfer_file = transfer_file;

    // Assign extended capture mode functions
    camera->functions->set_multi_exposure_mode = set_multi_exposure_mode;
    camera->functions->set_bracketing_mode = set_bracketing_mode;
    camera->functions->set_interval_shooting_mode = set_interval_shooting_mode;

    camera->functions->trigger_autofocus = trigger_autofocus;

    // Initialization completed successfully
    return GP_OK;
}

// Device Initialization and Identification
static int _camera_id(CameraText *id) {
    strncpy(id->text, "Pentax Driver", sizeof(id->text) - 1);
    id->text[sizeof(id->text) - 1] = '\0';
    return GP_OK;
}

static int _camera_abilities(CameraAbilitiesList *list) {
    CameraAbilities a;
    memset(&a, 0, sizeof(a));

    strcpy(a.model, "Pentax-PentaxPlugin");
    a.status = GP_DRIVER_STATUS_EXPERIMENTAL;
    a.port = GP_PORT_USB;
    a.operations = GP_OPERATION_CAPTURE_IMAGE | GP_OPERATION_CONFIG;
    a.file_operations = GP_FILE_OPERATION_NONE;
    a.folder_operations = GP_FOLDER_OPERATION_NONE;

    a.usb_vendor = USB_VENDOR_ID_PENTAX;
    a.usb_product = USB_PRODUCT_ID_PENTAX_K1II;
    gp_abilities_list_append(list, a);

    a.usb_product = USB_PRODUCT_ID_PENTAX_K3III;
    gp_abilities_list_append(list, a);

    a.usb_product = USB_PRODUCT_ID_PENTAX_K01;
    gp_abilities_list_append(list, a);

    return GP_OK;
}


// High-Resolution Live View
static int get_high_res_live_view(Camera *camera, CameraFile *file, GPContext *context) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_GET_LIVE_VIEW_FRAME_DATA);
    PtpContainer *response = NULL;
    if (!request) return GP_ERROR_NO_MEMORY;

    int ret = gp_ptp_transfer(camera, request, &response, context);
    gp_ptp_container_free(request);

    if (ret >= GP_OK && response) {
        ushort response_code = gp_ptp_get_response_code(response);
        int error_check = handle_mtp_response(response_code);
        if (error_check != GP_OK) {
            gp_ptp_container_free(response);
            return error_check;
        }
        gp_file_set_data_and_size(file, (char *)response->Payload.Data, response->Payload.Length);
        gp_ptp_container_free(response);
    }
    return ret;
}


// Astrotracer Functions
static int enable_astrotracer(Camera *camera, GPContext *context) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_ENABLE_ASTROTRACER);
    if (!request) return GP_ERROR_NO_MEMORY;

    gp_ptp_container_add_uint32(request, 1); // Enable Astrotracer
    int ret = gp_ptp_transfer(camera, request, NULL, context);
    gp_ptp_container_free(request);

    if (ret >= GP_OK) {
        ushort response_code = gp_ptp_get_response_code(NULL);
        return handle_mtp_response(response_code);
    }
    return ret;
}


static int disable_astrotracer(Camera *camera, GPContext *context) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_ENABLE_ASTROTRACER);
    if (!request) return GP_ERROR_NO_MEMORY;

    gp_ptp_container_add_uint32(request, 0); // Disable Astrotracer
    int ret = gp_ptp_transfer(camera, request, NULL, context);
    gp_ptp_container_free(request);
    if (ret >= GP_OK) {
        ushort response_code = gp_ptp_get_response_code(NULL);
        return handle_mtp_response(response_code);
    }
    return ret;
}

static int transfer_file(Camera *camera, GPContext *context, uint32_t object_handle, CameraFile *file) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_GET_OBJECT);
    if (!request) return GP_ERROR_NO_MEMORY;

    gp_ptp_container_add_uint32(request, object_handle);
    PtpContainer *response = NULL;
    int ret = gp_ptp_transfer(camera, request, &response, context);
    gp_ptp_container_free(request); // Free the request after sending

    if (ret >= GP_OK && response) {
        ushort response_code = gp_ptp_get_response_code(response);
        int error_check = handle_mtp_response(response_code);
        if (error_check != GP_OK) {
            gp_log(GP_LOG_ERROR, "Pentax Driver", "File transfer failed. Object Handle: %d", object_handle);
            gp_ptp_container_free(response); // Free the response before returning error
            return error_check;
        }
        // Set the file data if everything is successful
        gp_file_set_data_and_size(file, (char *)response->Payload.Data, response->Payload.Length);
        gp_ptp_container_free(response); // Free the response after use
        return GP_OK; // Return success
    }

    // Handle the case where response is NULL or transfer failed
    if (response) gp_ptp_container_free(response); // Ensure response is freed if allocated
    gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to transfer file. Object Handle: %d", object_handle);
    return ret; // Return the transfer error
}



// File Transfer and Storage
static int get_storage_info(Camera *camera, GPContext *context) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_GET_STORAGE_INFO);
    PtpContainer *response = NULL;
    if (!request) return GP_ERROR_NO_MEMORY;

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

static int capture_image(Camera *camera, GPContext *context) {
    CameraWidget *widget = NULL;
    CameraWidget *shutter_speed_widget = NULL;
    const char *shutter_speed_value;

    // Get the camera configuration
    int ret = gp_camera_get_config(camera, &widget, context);
    if (ret < GP_OK) {
        gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to get camera config.");
        return ret;
    }

    // Locate the "Shutter Speed" setting
    ret = gp_widget_get_child_by_name(widget, "shutterspeed", &shutter_speed_widget);
    if (ret < GP_OK) {
        gp_log(GP_LOG_ERROR, "Pentax Driver", "Shutter speed not found.");
        gp_widget_free(widget);
        return ret;
    }

    // Get the current shutter speed
    ret = gp_widget_get_value(shutter_speed_widget, &shutter_speed_value);
    if (ret < GP_OK) {
        gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to get shutter speed value.");
        gp_widget_free(widget);
        return ret;
    }

    // Check if we are in Bulb Mode
    if (strcmp(shutter_speed_value, "B") == 0) {
        gp_log(GP_LOG_INFO, "Pentax Driver", "Bulb mode detected. Starting exposure.");

        // Start Bulb exposure
        ret = start_bulb_exposure(camera, context);
        if (ret < GP_OK) {
            gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to start Bulb exposure.");
            gp_widget_free(widget);
            return ret;
        }

        // Wait for the user to trigger stop (or wait for libgphoto2 event)
        gp_log(GP_LOG_INFO, "Pentax Driver", "Waiting for Bulb exposure to be stopped.");
	// Wait for capture event to complete (handles errors properly)
	ret = gp_camera_wait_for_event(camera, GP_EVENT_CAPTURE_COMPLETE, context);
	if (ret < GP_OK) {
    		gp_log(GP_LOG_ERROR, "Pentax Driver", "Capture event failed, stopping Bulb exposure.");
    		stop_bulb_exposure(camera, context);  // Stop exposure to prevent the camera from getting stuck
   		 gp_widget_free(widget);
   	 return ret;  // Return failure so that libgphoto2 knows something went wrong
}

        // Stop Bulb exposure
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

    // If not Bulb mode, perform standard capture
    gp_widget_free(widget);
    return gp_camera_capture(camera, GP_CAPTURE_IMAGE, NULL, context);
}


// Extended Capture Modes
static int set_multi_exposure_mode(Camera *camera, GPContext *context, uint32_t mode) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_SET_CAPTURE_MODE);
    if (!request) return GP_ERROR_NO_MEMORY;

    gp_ptp_container_add_uint32(request, mode); // Multi-exposure mode
    int ret = gp_ptp_transfer(camera, request, NULL, context);
    gp_ptp_container_free(request);

    if (ret >= GP_OK) {
        ushort response_code = gp_ptp_get_response_code(NULL);
        return handle_mtp_response(response_code);
    }
    return ret;
}

// Extended Capture Image Function
static int capture_image(Camera *camera, GPContext *context, uint32_t capture_mode) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_SET_CAPTURE_MODE);
    if (!request) return GP_ERROR_NO_MEMORY;

    // Set capture mode (e.g., single-shot, burst)
    gp_ptp_container_add_uint32(request, capture_mode);
    int ret = gp_ptp_transfer(camera, request, NULL, context);
    gp_ptp_container_free(request);

    if (ret >= GP_OK) {
        // Proceed to capture the image
        return capture_image(camera, context);
    }

    gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to set capture mode: %d", capture_mode);
    return ret;
}



static int set_bracketing_mode(Camera *camera, GPContext *context, uint32_t mode) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_SET_CAPTURE_MODE);
    if (!request) return GP_ERROR_NO_MEMORY;

    gp_ptp_container_add_uint32(request, mode); // Bracketing mode
    int ret = gp_ptp_transfer(camera, request, NULL, context);
    gp_ptp_container_free(request);

    if (ret >= GP_OK) {
        ushort response_code = gp_ptp_get_response_code(NULL);
        return handle_mtp_response(response_code);
    }
    return ret;
}

static int set_interval_shooting_mode(Camera *camera, GPContext *context, uint32_t interval_seconds) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_SET_INTERVAL_MODE);
    if (!request) return GP_ERROR_NO_MEMORY;

    gp_ptp_container_add_uint32(request, interval_seconds); // Interval shooting mode
    int ret = gp_ptp_transfer(camera, request, NULL, context);
    gp_ptp_container_free(request);

    if (ret >= GP_OK) {
        ushort response_code = gp_ptp_get_response_code(NULL);
        return handle_mtp_response(response_code);
    }
    return ret;
}

static int stop_live_view(Camera *camera, GPContext *context) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_STOP_LIVE_VIEW);
    if (!request) return GP_ERROR_NO_MEMORY;

    int ret = gp_ptp_transfer(camera, request, NULL, context);
    gp_ptp_container_free(request);
    return ret;
}

static int trigger_autofocus(Camera *camera, GPContext *context) {
    // Step 1: Trigger Autofocus
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_AF_TRIGGER);
    if (!request) return GP_ERROR_NO_MEMORY;

    // Send the autofocus trigger command
    PtpContainer *response = NULL;
    int ret = gp_ptp_transfer(camera, request, &response, context);
    gp_ptp_container_free(request);

    // Step 2: Handle Response
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

    // Step 3: (Optional) Validate Autofocus Status
    request = gp_ptp_create_request(PTP_VENDOR_PENTAX_GET_AF_STATUS);
    if (!request) return GP_ERROR_NO_MEMORY;

    ret = gp_ptp_transfer(camera, request, &response, context);
    gp_ptp_container_free(request);

    if (ret >= GP_OK && response) {
        ushort af_status = response->Payload.Data[0]; // Assuming first byte contains status
        gp_ptp_container_free(response);

        if (af_status == 1) { // 1 indicates success (example based on assumed response structure)
            gp_log(GP_LOG_INFO, "Pentax Driver", "Autofocus successful.");
            return GP_OK;
        } else {
            gp_log(GP_LOG_ERROR, "Pentax Driver", "Autofocus failed. Status: %d", af_status);
            return GP_ERROR;
        }
    }

    gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to get autofocus status.");
    return ret;
}

static int set_astrotracer_mode(Camera *camera, GPContext *context, uint32_t mode) {
    // Create a PTP request for setting Astrotracer mode
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_SET_ASTROTRACER_MODE);
    if (!request) return GP_ERROR_NO_MEMORY;

    // Add the mode parameter to the request
    gp_ptp_container_add_uint32(request, mode); // Mode: 1 = Enable, 0 = Disable (example)

    // Send the request
    int ret = gp_ptp_transfer(camera, request, NULL, context);
    gp_ptp_container_free(request);

    // Check the response
    if (ret >= GP_OK) {
        ushort response_code = gp_ptp_get_response_code(NULL);
        return handle_mtp_response(response_code);
    }

    gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to set Astrotracer mode.");
    return ret;
}

static int set_astrotracer_time_limit(Camera *camera, GPContext *context, uint32_t time_limit) {
    // Create a PTP request for setting the Astrotracer time limit
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_SET_ASTROTRACER_TIME_LIMIT);
    if (!request) return GP_ERROR_NO_MEMORY;

    // Add the time limit parameter to the request
    gp_ptp_container_add_uint32(request, time_limit); // Time limit in seconds (example)

    // Send the request
    int ret = gp_ptp_transfer(camera, request, NULL, context);
    gp_ptp_container_free(request);

    // Check the response
    if (ret >= GP_OK) {
        ushort response_code = gp_ptp_get_response_code(NULL);
        return handle_mtp_response(response_code);
    }

    gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to set Astrotracer time limit.");
    return ret;
}

static int start_bulb_exposure(Camera *camera, GPContext *context) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_BULB_START);
    if (!request) return GP_ERROR_NO_MEMORY;

    int ret = gp_ptp_transfer(camera, request, NULL, context);
    gp_ptp_container_free(request);

    if (ret >= GP_OK) {
        ushort response_code = gp_ptp_get_response_code(NULL);
        return handle_mtp_response(response_code);
    }

    gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to start Bulb exposure.");
    return ret;
}

static int stop_bulb_exposure(Camera *camera, GPContext *context) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_BULB_STOP);
    if (!request) return GP_ERROR_NO_MEMORY;

    int ret = gp_ptp_transfer(camera, request, NULL, context);
    gp_ptp_container_free(request);

    if (ret >= GP_OK) {
        ushort response_code = gp_ptp_get_response_code(NULL);
        return handle_mtp_response(response_code);
    }

    gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to stop Bulb exposure.");
    return ret;
}


static int set_bulb_timer(Camera *camera, GPContext *context, uint32_t time_seconds) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_SET_BULB_TIMER);
    if (!request) return GP_ERROR_NO_MEMORY;

    gp_ptp_container_add_uint32(request, time_seconds);
    int ret = gp_ptp_transfer(camera, request, NULL, context);
    gp_ptp_container_free(request);

    if (ret >= GP_OK) {
        ushort response_code = gp_ptp_get_response_code(NULL);
        return handle_mtp_response(response_code);
    }

    gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to set Bulb timer.");
    return ret;
}

static int set_exposure_mode(Camera *camera, GPContext *context, uint32_t mode) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_SET_EXPOSURE_MODE);
    if (!request) return GP_ERROR_NO_MEMORY;

    gp_ptp_container_add_uint32(request, mode); // Mode ID (9 for Bulb)
    int ret = gp_ptp_transfer(camera, request, NULL, context);
    gp_ptp_container_free(request);

    if (ret >= GP_OK) {
        ushort response_code = gp_ptp_get_response_code(NULL);
        return handle_mtp_response(response_code);
    }

    gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to set exposure mode: %d", mode);
    return ret;
}


static int adjust_focus(Camera *camera, GPContext *context, int step) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_FOCUS_STEP);
    if (!request) return GP_ERROR_NO_MEMORY;

    gp_ptp_container_add_int32(request, step); // Positive = forward, Negative = backward
    int ret = gp_ptp_transfer(camera, request, NULL, context);
    gp_ptp_container_free(request);

    if (ret >= GP_OK) {
        ushort response_code = gp_ptp_get_response_code(NULL);
        return handle_mtp_response(response_code);
    }

    gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to adjust focus by step: %d", step);
    return ret;
}

static int set_focus_position(Camera *camera, GPContext *context, uint32_t position) {
    PtpContainer *request = gp_ptp_create_request(PTP_VENDOR_PENTAX_SET_FOCUS_POSITION);
    if (!request) return GP_ERROR_NO_MEMORY;

    gp_ptp_container_add_uint32(request, position);
    int ret = gp_ptp_transfer(camera, request, NULL, context);
    gp_ptp_container_free(request);

    if (ret >= GP_OK) {
        ushort response_code = gp_ptp_get_response_code(NULL);
        return handle_mtp_response(response_code);
    }

    gp_log(GP_LOG_ERROR, "Pentax Driver", "Failed to set focus position to: %d", position);
    return ret;
}


static int trigger_autofocus(Camera *camera, GPContext *context) {
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

