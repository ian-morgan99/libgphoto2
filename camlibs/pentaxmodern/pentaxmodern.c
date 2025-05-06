#include "config.h"
#include <gphoto2/gphoto2-library.h>
#include <gphoto2/gphoto2-port.h>
#include <gphoto2/gphoto2-result.h>
#include <gphoto2/gphoto2-setting.h>
#include <gphoto2/ptp.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <gphoto2/gphoto2-log.h>

/* Added after review Ian Morgan github@morgan-multinational.co.uk
#include <gphoto2-gpl.h>
#include "pentaxmodern.h"

#define PENTAX_VENDOR_ID 0x0A17
#define PENTAX_PRODUCT_K1II 0x503A
#define PENTAX_PRODUCT_K3III 0x5049
#define PENTAX_PRODUCT_K70 0x503E

#define PENTAX_OC_START_LIVE_VIEW 0x9201
#define PENTAX_OC_END_LIVE_VIEW 0x9202
#define PENTAX_OC_GET_LV_FRAME 0x9203
#define PENTAX_OC_GET_ISO          0x9501
#define PENTAX_OC_SET_ISO          0x9502
#define PENTAX_OC_GET_SHUTTER      0x9503
#define PENTAX_OC_SET_SHUTTER      0x9504
#define PENTAX_OC_GET_APERTURE     0x9505
#define PENTAX_OC_SET_APERTURE     0x9506
#define PENTAX_OC_GET_DRIVE_MODE   0x9507
#define PENTAX_OC_SET_DRIVE_MODE   0x9508

#define PENTAX_OC_AF_TRIGGER 0x9101
#define PENTAX_OC_ENABLE_ASTROTRACER 0x9401
#define PENTAX_OC_DISABLE_ASTROTRACER 0x9402
#define PENTAX_OC_SET_ASTROTRACER_TIME 0x9403
#define PENTAX_OC_ENABLE_MIRROR_UP 0x9404
#define PENTAX_OC_DISABLE_MIRROR_UP 0x9405
#define PENTAX_OC_LED_ON            0x9410
#define PENTAX_OC_LED_OFF           0x9411
#define PENTAX_OC_GPS_SYNC          0x9412
#define PENTAX_OC_GET_WHITE_BALANCE 0x9510
#define PENTAX_OC_SET_WHITE_BALANCE 0x9511
#define PENTAX_OC_GET_EXP_COMP      0x9512
#define PENTAX_OC_SET_EXP_COMP      0x9513
#define PENTAX_OC_GET_IMAGE_QUALITY 0x9514
#define PENTAX_OC_SET_IMAGE_QUALITY 0x9515
#define PENTAX_OC_GET_METERING_MODE 0x9516
#define PENTAX_OC_SET_METERING_MODE 0x9517
#define PENTAX_OC_GET_FLASH_MODE    0x9518
#define PENTAX_OC_SET_FLASH_MODE    0x9519
#define PENTAX_OC_GET_FOCUS_MODE    0x951A
#define PENTAX_OC_SET_FOCUS_MODE    0x951B
#define PENTAX_OC_GET_BATTERY_LEVEL 0x951C
#define PENTAX_OC_TRIGGER_CLEAN_MIRROR 0x951D
#define PENTAX_OC_GET_IMAGE_SHIFT 0x951E
#define PENTAX_OC_SET_IMAGE_SHIFT 0x951F
#define PENTAX_OC_GET_HDR_MODE 0x9520
#define PENTAX_OC_SET_HDR_MODE 0x9521
#define PENTAX_OC_GET_BRACKETING_MODE 0x9522
#define PENTAX_OC_SET_BRACKETING_MODE 0x9523
#define PENTAX_OC_GET_PEAKING 0x9526
#define PENTAX_OC_SET_PEAKING 0x9527
#define PENTAX_OC_GET_FACE_DETECT 0x9528
#define PENTAX_OC_SET_FACE_DETECT 0x9529
#define PENTAX_OC_GET_STORAGE_IDS 0x9530
#define PENTAX_OC_SET_ACTIVE_STORAGE 0x9531
#define PENTAX_OC_GET_WRITE_MODE 0x9532
#define PENTAX_OC_SET_WRITE_MODE 0x9533

/* Added Ian Morgan github@morgan-multinational.co.uk
#ifndef GP_PENTAX_MODERN_H
#define GP_PENTAX_MODERN_H
… 
#endif
	
typedef struct {
    GPPort *port;
    int model;
    gboolean astrotracer_enabled;
    gboolean bulb_mode_active;
    gboolean live_view_active;
    gboolean shift_mode_enabled;
    int live_view_interval;
    int astrotracer_time_limit;
} CameraPrivate;

static int pentax_start_live_view(PTPParams *params) {
    return ptp_generic_no_data(params, PENTAX_OC_START_LIVE_VIEW, 0, 0, 0, 0, 0);
}

static int pentax_end_live_view(PTPParams *params) {
    return ptp_generic_no_data(params, PENTAX_OC_END_LIVE_VIEW, 0, 0, 0, 0, 0);
}

static int pentax_get_lv_frame(PTPParams *params, char **data, unsigned long *size) {
    return ptp_generic_get_data(params, PENTAX_OC_GET_LV_FRAME, data, size);
}

static int camera_init(Camera *camera, GPContext *context) {
    CameraPrivate *priv = calloc(1, sizeof(CameraPrivate));
    camera->pl = priv;

    gp_port_set_timeout(camera->port, 5000);

    priv->astrotracer_enabled = FALSE;
    priv->bulb_mode_active = FALSE;
    priv->live_view_active = FALSE;
    priv->astrotracer_time_limit = 60;

    if (gp_widget_get_child_by_name(window, "LED On", &widget) == GP_OK) {
        gp_widget_get_value(widget, &enabled);
        pentax_set_led(camera, enabled);
    }
    if (gp_widget_get_child_by_name(window, "Trigger GPS Sync", &widget) == GP_OK) {
        pentax_trigger_gps_sync(camera);
    }

    if (gp_widget_get_child_by_name(window, "Exposure Compensation", &widget) == GP_OK) {
        gp_widget_get_value(widget, &new_val);
        PTPParams params;
        ptp_init(&params, camera->port);
        uint32_t comp = lookup_value(exp_comp_table, new_val);
        ptp_generic_no_data(&params, PENTAX_OC_SET_EXP_COMP, comp, 0, 0, 0, 0);
    }
    if (gp_widget_get_child_by_name(window, "Mirror Lock-up for Cleaning", &widget) == GP_OK) {
        PTPParams params;
        ptp_init(&params, camera->port);
        pentax_send_simple_command(&params, PENTAX_OC_TRIGGER_CLEAN_MIRROR);
    }
    if (gp_widget_get_child_by_name(window, "Image Shift Mode", &widget) == GP_OK) {
        gp_widget_get_value(widget, &new_val);
        PTPParams params;
        ptp_init(&params, camera->port);
        uint32_t shift_mode = lookup_value(shift_mode_table, new_val);
        ptp_generic_no_data(&params, PENTAX_OC_SET_IMAGE_SHIFT, shift_mode, 0, 0, 0, 0);
    }
    if (gp_widget_get_child_by_name(window, "HDR Mode", &widget) == GP_OK) {
        gp_widget_get_value(widget, &new_val);
        PTPParams params;
        ptp_init(&params, camera->port);
        uint32_t hdr = lookup_value(hdr_mode_table, new_val);
        ptp_generic_no_data(&params, PENTAX_OC_SET_HDR_MODE, hdr, 0, 0, 0, 0);
    }
    if (gp_widget_get_child_by_name(window, "Bracketing Mode", &widget) == GP_OK) {
        gp_widget_get_value(widget, &new_val);
        PTPParams params;
        ptp_init(&params, camera->port);
        uint32_t bracket = lookup_value(bracketing_mode_table, new_val);
        ptp_generic_no_data(&params, PENTAX_OC_SET_BRACKETING_MODE, bracket, 0, 0, 0, 0);
    }
    if (gp_widget_get_child_by_name(window, "Focus Mode", &widget) == GP_OK) {
        gp_widget_get_value(widget, &new_val);
        PTPParams params;
        ptp_init(&params, camera->port);
        uint32_t focus = lookup_value(focus_table, new_val);
        ptp_generic_no_data(&params, PENTAX_OC_SET_FOCUS_MODE, focus, 0, 0, 0, 0);
    }
    if (gp_widget_get_child_by_name(window, "Focus Peaking Enabled", &widget) == GP_OK) {
        gp_widget_get_value(widget, &enabled);
        PTPParams params;
        ptp_init(&params, camera->port);
        ptp_generic_no_data(&params, PENTAX_OC_SET_PEAKING, enabled ? 1 : 0, 0, 0, 0, 0);
    }
    if (gp_widget_get_child_by_name(window, "Face/Eye Detection Enabled", &widget) == GP_OK) {
        gp_widget_get_value(widget, &enabled);
        PTPParams params;
        ptp_init(&params, camera->port);
        ptp_generic_no_data(&params, PENTAX_OC_SET_FACE_DETECT, enabled ? 1 : 0, 0, 0, 0, 0);
    }

    return GP_OK;
}

static int pentax_set_led(Camera *camera, gboolean on) {
    PTPParams params;
    ptp_init(&params, camera->port);
    uint16_t code = on ? PENTAX_OC_LED_ON : PENTAX_OC_LED_OFF;
    return pentax_send_simple_command(&params, code);
}

static int pentax_trigger_gps_sync(Camera *camera) {
    PTPParams params;
    ptp_init(&params, camera->port);
    return pentax_send_simple_command(&params, PENTAX_OC_GPS_SYNC);
}

static int pentax_set_mirror_up(Camera *camera, gboolean enable) {
    PTPParams params;
    ptp_init(&params, camera->port);
    uint16_t code = enable ? PENTAX_OC_ENABLE_MIRROR_UP : PENTAX_OC_DISABLE_MIRROR_UP;
    return pentax_send_simple_command(&params, code);
}

static int pentax_send_simple_command(PTPParams *params, uint16_t code) {
    int ret = ptp_generic_no_data(params, code, 0, 0, 0, 0, 0);
    gp_log(GP_LOG_DEBUG, "pentax", "Sent vendor-specific command: 0x%X, ret=0x%X", code, ret);
    return (ret == PTP_RC_OK) ? GP_OK : GP_ERROR;
}

static int pentax_set_astrotracer_time(PTPParams *params, uint32_t seconds) {
    int ret = ptp_generic_no_data(params, PENTAX_OC_SET_ASTROTRACER_TIME, seconds, 0, 0, 0, 0);
    gp_log(GP_LOG_DEBUG, "pentax", "Set Astrotracer time to %u sec, ret=0x%X", seconds, ret);
    return (ret == PTP_RC_OK) ? GP_OK : GP_ERROR;
}

static int camera_exit(Camera *camera, GPContext *context) {
    CameraPrivate *priv = camera->pl;
    CameraWidget *widget;
    char *new_val = NULL;

    if (gp_widget_get_child_by_name(window, "Active SD Card", &widget) == GP_OK) {
        gp_widget_get_value(widget, &new_val);
        uint32_t slot = (strcmp(new_val, "Card 1") == 0) ? 1 : 2;
        PTPParams params;
        ptp_init(&params, camera->port);
        ptp_generic_no_data(&params, PENTAX_OC_SET_ACTIVE_STORAGE, slot, 0, 0, 0, 0);
    }
    if (gp_widget_get_child_by_name(window, "Write Mode", &widget) == GP_OK) {
        gp_widget_get_value(widget, &new_val);
        uint32_t mode = 0;
        if (strcmp(new_val, "Sequential") == 0) mode = 0;
        else if (strcmp(new_val, "Parallel") == 0) mode = 1;
        else if (strcmp(new_val, "RAW to Card 1, JPEG to Card 2") == 0) mode = 2;
        PTPParams params;
        ptp_init(&params, camera->port);
        ptp_generic_no_data(&params, PENTAX_OC_SET_WRITE_MODE, mode, 0, 0, 0, 0);
    }

    if (gp_widget_get_child_by_name(window, "Live View Zoom", &widget) == GP_OK) {
        gp_widget_get_value(widget, &new_val);
        uint32_t zoom = 0;
        if (strcmp(new_val, "1x") == 0) zoom = 1;
        else if (strcmp(new_val, "2x") == 0) zoom = 2;
        else if (strcmp(new_val, "4x") == 0) zoom = 4;
        else if (strcmp(new_val, "6x") == 0) zoom = 6;
        else if (strcmp(new_val, "8x") == 0) zoom = 8;
        PTPParams params;
        ptp_init(&params, camera->port);
        ptp_generic_no_data(&params, 0x9525, zoom, 0, 0, 0, 0);
    }
    if (priv->live_view_active) {
        PTPParams params;
        ptp_init(&params, camera->port);
    gp_log(GP_LOG_DEBUG, "pentax", "Initialized PTP session");
        pentax_end_live_view(&params);
    }
    free(priv);
    return GP_OK;
}

static int camera_capture(Camera *camera, CameraCaptureType type, CameraFilePath *path, GPContext *context) {
    CameraPrivate *priv = camera->pl;

    if (type != GP_CAPTURE_IMAGE) return GP_ERROR_NOT_SUPPORTED;

    PTPParams params;
    ptp_init(&params, camera->port);

    int ret;
    if (priv->bulb_mode_active) {
        ret = ptp_custom_command(&params, 0x9205);
        gp_log(GP_LOG_DEBUG, "pentax", "Started bulb mode");
        sleep(priv->astrotracer_time_limit);
        ret = ptp_custom_command(&params, 0x9206);
        gp_log(GP_LOG_DEBUG, "pentax", "Ended bulb mode");
    } else {
        ret = ptp_initiate_capture(&params, 0, 0);
    }

    if (ret != PTP_RC_OK) {
        gp_log(GP_LOG_ERROR, "pentax", "PTP operation failed with code: 0x%X", ret);
        return GP_ERROR;
    }

    strcpy(path->folder, "/");
    strcpy(path->name, "capt0001.dng");

    return GP_OK;
}

static int camera_get_preview(Camera *camera, CameraFile *file, GPContext *context) {
    CameraPrivate *priv = camera->pl;
    if (!priv->live_view_active) return GP_ERROR_BAD_PARAMETERS;

    PTPParams params;
    ptp_init(&params, camera->port);

    char *data = NULL;
    unsigned long size = 0;
    int ret = pentax_get_lv_frame(&params, &data, &size);
    if (ret != PTP_RC_OK || !data || size == 0) return GP_ERROR;

    gp_file_set_data_and_size(file, data, size);
    return GP_OK;
}

typedef struct {
    const char *label;
    uint32_t value;
} PentaxLookupEntry;

static PentaxLookupEntry iso_table[] = {
    {"AUTO", 0}, {"100", 100}, {"125", 125}, {"160", 160},
    {"200", 200}, {"250", 250}, {"320", 320}, {"400", 400},
    {"500", 500}, {"640", 640}, {"800", 800}, {"1000", 1000},
    {"1250", 1250}, {"1600", 1600}, {"2000", 2000}, {"2500", 2500},
    {"3200", 3200}, {"4000", 4000}, {"5000", 5000}, {"6400", 6400},
    {"8000", 8000}, {"10000", 10000}, {"12800", 12800}, {"16000", 16000},
    {"20000", 20000}, {"25600", 25600}, {"32000", 32000}, {"40000", 40000},
    {"51200", 51200}, {"64000", 64000}, {"80000", 80000}, {"102400", 102400},
    {"128000", 128000}, {"160000", 160000}, {"204800", 204800}, {"256000", 256000},
    {"320000", 320000}, {"409600", 409600}, {"512000", 512000}, {"640000", 640000},
    {"819200", 819200},
    {NULL, 0}
};

static PentaxLookupEntry shutter_table[] = {
    {"1/24000", 24000}, {"1/20000", 20000}, {"1/16000", 16000}, {"1/12800", 12800},
    {"1/10000", 10000}, {"1/8000", 8000}, {"1/6400", 6400}, {"1/6000", 6000},
    {"1/5000", 5000}, {"1/4000", 4000}, {"1/3200", 3200}, {"1/3000", 3000},
    {"1/2500", 2500}, {"1/2000", 2000}, {"1/1600", 1600}, {"1/1500", 1500},
    {"1/1250", 1250}, {"1/1000", 1000}, {"1/800", 800}, {"1/750", 750},
    {"1/640", 640}, {"1/500", 500}, {"1/400", 400}, {"1/350", 350},
    {"1/320", 320}, {"1/250", 250}, {"1/200", 200}, {"1/180", 180},
    {"1/160", 160}, {"1/125", 125}, {"1/100", 100}, {"1/90", 90},
    {"1/80", 80}, {"1/60", 60}, {"1/50", 50}, {"1/45", 45},
    {"1/40", 40}, {"1/30", 30}, {"1/25", 25}, {"1/20", 20},
    {"1/15", 15}, {"1/13", 13}, {"1/10", 10}, {"1/8", 8},
    {"1/6", 6}, {"1/5", 5}, {"1/4", 4}, {"1/3", 3},
    {"1/2", 2}, {"1\"", 1}, {"2\"", -2}, {"3\"", -3},
    {"4\"", -4}, {"5\"", -5}, {"6\"", -6}, {"8\"", -8},
    {"10\"", -10}, {"13\"", -13}, {"15\"", -15}, {"20\"", -20},
    {"25\"", -25}, {"30\"", -30},
    {NULL, 0}
};

static PentaxLookupEntry aperture_table[] = {
    {"f/1.0", 10}, {"f/1.1", 11}, {"f/1.2", 12}, {"f/1.4", 14},
    {"f/1.6", 16}, {"f/1.8", 18}, {"f/2.0", 20}, {"f/2.2", 22},
    {"f/2.5", 25}, {"f/2.8", 28}, {"f/3.2", 32}, {"f/3.5", 35},
    {"f/4.0", 40}, {"f/4.5", 45}, {"f/5.0", 50}, {"f/5.6", 56},
    {"f/6.3", 63}, {"f/7.1", 71}, {"f/8.0", 80}, {"f/9.0", 90},
    {"f/10", 100}, {"f/11", 110}, {"f/13", 130}, {"f/14", 140},
    {"f/16", 160}, {"f/18", 180}, {"f/20", 200}, {"f/22", 220},
    {"f/25", 250}, {"f/29", 290}, {"f/32", 320}, {"f/36", 360},
    {"f/40", 400}, {"f/45", 450}, {"f/51", 510}, {"f/57", 570},
    {"f/64", 640},
    {NULL, 0}
};

static PentaxLookupEntry drive_table[] = {
    {"Single", 1}, {"Continuous High", 2}, {"Continuous Low", 3}, {"Timer", 4},
    {NULL, 0}
};

static PentaxLookupEntry wb_table[] = {
    {"Auto", 1}, {"Daylight", 2}, {"Shade", 3}, {"Cloudy", 4}, {"Tungsten", 5},
    {"Fluorescent", 6}, {"Manual", 7},
    {NULL, 0}
};

static PentaxLookupEntry quality_table[] = {
    {"RAW", 1}, {"JPEG", 2}, {"RAW+JPEG", 3},
    {NULL, 0}
};

static PentaxLookupEntry metering_table[] = {
    {"Evaluative", 1}, {"Center-weighted", 2}, {"Spot", 3},
    {NULL, 0}
};

static PentaxLookupEntry flash_table[] = {
    {"Auto", 1}, {"On", 2}, {"Off", 3},
    {NULL, 0}
};

static PentaxLookupEntry focus_table[] = {
    {"AF-S", 1}, {"AF-C", 2}, {"Manual", 3},
    {"Contrast AF", 4}, {"Phase Detect", 5}, {"Face Detect", 6},
    {"Touch AF", 7}, {"Focus Peaking", 8}, {"Zoom Focus Assist", 9},
    {NULL, 0}
};

static PentaxLookupEntry exp_comp_table[] = {
    {"-5.0", -50}, {"-4.7", -47}, {"-4.3", -43}, {"-4.0", -40},
    {"-3.7", -37}, {"-3.3", -33}, {"-3.0", -30}, {"-2.7", -27},
    {"-2.3", -23}, {"-2.0", -20}, {"-1.7", -17}, {"-1.3", -13},
    {"-1.0", -10}, {"-0.7", -7}, {"-0.3", -3}, {"0.0", 0},
    {"+0.3", 3}, {"+0.7", 7}, {"+1.0", 10}, {"+1.3", 13},
    {"+1.7", 17}, {"+2.0", 20}, {"+2.3", 23}, {"+2.7", 27},
    {"+3.0", 30}, {"+3.3", 33}, {"+3.7", 37}, {"+4.0", 40},
    {"+4.3", 43}, {"+4.7", 47}, {"+5.0", 50},
    {NULL, 0}
};

static PentaxLookupEntry shift_mode_table[] = {
    {"Off", 0},
    {"Standard Shift", 1},
    {"Pixel Shift", 2},
    {"Pixel Shift + Motion Correction", 3},
    {NULL, 0}
};

static PentaxLookupEntry hdr_mode_table[] = {
    {"Off", 0},
    {"Auto HDR", 1},
    {"HDR1", 2},
    {"HDR2", 3},
    {"HDR3", 4},
    {NULL, 0}
};

static PentaxLookupEntry bracketing_mode_table[] = {
    {"Off", 0},
    {"1 EV step", 1},
    {"0.7 EV step", 2},
    {"0.5 EV step", 3},
    {"0.3 EV step", 4},
    {NULL, 0}
};

static PentaxLookupEntry image_tone_table[] = {
    {"Natural", 0}, {"Bright", 1}, {"Portrait", 2}, {"Landscape", 3},
    {"Vibrant", 4}, {"Flat", 5}, {"Monotone", 6}, {"Muted Color", 7},
    {"Reversal Film", 8}, {"Bleach Bypass", 9}, {"Cross Process", 10},
    {"Satobi", 11}, {"Silky", 12}, {NULL, 0}
};

static PentaxLookupEntry ci_mode_table[] = {
    {"Standard", 0}, {"Bright", 1}, {"Portrait", 2}, {"Landscape", 3},
    {"Vibrant", 4}, {"Monotone", 5}, {"Radiant", 6}, {"Muted", 7},
    {"Reversal Film", 8}, {"Bleach Bypass", 9}, {"Cross Process", 10},
    {"Satobi", 11}, {"Silky", 12}, {"Muted Color", 13}, {NULL, 0}
};

static PentaxLookupEntry scene_mode_table[] = {
    {"Auto", 0}, {"Portrait", 1}, {"Landscape", 2}, {"Macro", 3},
    {"Sports", 4}, {"Night Scene", 5}, {"Pet", 6}, {"Sunset", 7},
    {"Blue Sky", 8}, {"Forest", 9}, {"Night Portrait", 10},
    {"Night Scene HDR", 11}, {"Kids", 12}, {"Surf & Snow", 13},
    {"Backlight Silhouette", 14}, {"Food", 15}, {"Stage Lighting", 16},
    {"Fireworks", 17}, {"Museum", 18}, {"Text", 19}, {"Moss", 20},
    {"Water Reflection", 21}, {"Light Trails", 22}, {"Underwater", 23},
    {NULL, 0}
};

static PentaxLookupEntry exposure_mode_table[] = {
    {"P", 0}, {"Sv", 1}, {"Tv", 2}, {"Av", 3},
    {"TAv", 4}, {"M", 5}, {"B", 6}, {"X", 7},
    {"Green", 8}, {"U1", 9}, {"U2", 10},
    {"Movie", 11}, {"Auto Picture", 12}, {"Scene Mode", 13},
    {NULL, 0}
};

static const char *lookup_label(PentaxLookupEntry *table, uint32_t val) {
    for (int i = 0; table[i].label; i++) {
        if (table[i].value == val) return table[i].label;
    }
    return "Unknown";
}

static uint32_t lookup_value(PentaxLookupEntry *table, const char *label) {
    for (int i = 0; table[i].label; i++) {
        if (strcmp(table[i].label, label) == 0) return table[i].value;
    }
    return 0;
}

static int camera_get_config(Camera *camera, CameraWidget **window, GPContext *context) {
    CameraPrivate *priv = camera->pl;
    CameraWidget *section, *widget_toggle, *range;
    PTPParams params;
    ptp_init(&params, camera->port);
    int enabled = 0;

    gp_widget_new(GP_WIDGET_WINDOW, "Pentax Camera Configuration", window);

    gp_widget_new(GP_WIDGET_SECTION, "Astrotracer", &section);
    gp_widget_append(*window, section);
    gp_widget_new(GP_WIDGET_TOGGLE, "Enable Astrotracer", &widget_toggle);
    gp_widget_set_value(widget_toggle, &priv->astrotracer_enabled);
    gp_widget_append(section, widget_toggle);

    gp_widget_new(GP_WIDGET_RANGE, "Astrotracer Time Limit (sec)", &range);
    gp_widget_set_range(range, 10, 300, 10);
    gp_widget_set_value(range, &priv->astrotracer_time_limit);
    gp_widget_append(section, range);

    gp_widget_new(GP_WIDGET_SECTION, "Capture Modes", &section);
    gp_widget_append(*window, section);
    gp_widget_new(GP_WIDGET_TOGGLE, "Bulb Mode Active", &widget_toggle);
    gp_widget_set_value(widget_toggle, &priv->bulb_mode_active);
    gp_widget_append(section, widget_toggle);

    gp_widget_new(GP_WIDGET_SECTION, "Live View", &section);
    gp_widget_append(*window, section);
    gp_widget_new(GP_WIDGET_TOGGLE, "Live View Active", &widget_toggle);
    gp_widget_set_value(widget_toggle, &priv->live_view_active);
    gp_widget_append(section, widget_toggle);
    gp_widget_new(GP_WIDGET_RANGE, "Live View Interval (ms)", &range);
    gp_widget_set_range(range, 30, 1000, 10);
    gp_widget_set_value(range, &priv->live_view_interval);
    gp_widget_append(section, range);

    gp_widget_new(GP_WIDGET_SECTION, "Exposure", &section);
    gp_widget_append(*window, section);

    // Exposure Mode
    gp_widget_new(GP_WIDGET_RADIO, "Exposure Mode", &widget_toggle);
    uint32_t exp_raw = 0;
    ptp_generic_get_uint32(&params, 0x9540, &exp_raw);
    const char *exp_label = lookup_label(exposure_mode_table, exp_raw);
    for (int i = 0; exposure_mode_table[i].label; i++) {
        gp_widget_add_choice(widget_toggle, exposure_mode_table[i].label);
    }
    gp_widget_set_value(widget_toggle, exp_label);
    gp_widget_append(section, widget_toggle);

    // Scene Mode
    gp_widget_new(GP_WIDGET_RADIO, "Scene Mode", &widget_toggle);
    uint32_t scn_raw = 0;
    ptp_generic_get_uint32(&params, 0x9541, &scn_raw);
    const char *scn_label = lookup_label(scene_mode_table, scn_raw);
    for (int i = 0; scene_mode_table[i].label; i++) {
        gp_widget_add_choice(widget_toggle, scene_mode_table[i].label);
    }
    gp_widget_set_value(widget_toggle, scn_label);
    gp_widget_append(section, widget_toggle);

    // Image Tone
    gp_widget_new(GP_WIDGET_RADIO, "Image Tone", &widget_toggle);
    uint32_t tone_raw = 0;
    ptp_generic_get_uint32(&params, 0x9542, &tone_raw);
    const char *tone_label = lookup_label(image_tone_table, tone_raw);
    for (int i = 0; image_tone_table[i].label; i++) {
        gp_widget_add_choice(widget_toggle, image_tone_table[i].label);
    }
    gp_widget_set_value(widget_toggle, tone_label);
    gp_widget_append(section, widget_toggle);

    // CI Mode
    gp_widget_new(GP_WIDGET_RADIO, "CI Mode", &widget_toggle);
    uint32_t ci_raw = 0;
    ptp_generic_get_uint32(&params, 0x9543, &ci_raw);
    const char *ci_label = lookup_label(ci_mode_table, ci_raw);
    for (int i = 0; ci_mode_table[i].label; i++) {
        gp_widget_add_choice(widget_toggle, ci_mode_table[i].label);
    }
    gp_widget_set_value(widget_toggle, ci_label);
    gp_widget_append(section, widget_toggle);
    gp_widget_append(*window, section);

    // HDR Mode
    gp_widget_new(GP_WIDGET_RADIO, "HDR Mode", &widget_toggle);
    uint32_t hdr_raw = 0;
    ptp_generic_get_uint32(&params, PENTAX_OC_GET_HDR_MODE, &hdr_raw);
    const char *hdr_label = lookup_label(hdr_mode_table, hdr_raw);
    for (int i = 0; hdr_mode_table[i].label; i++) {
        gp_widget_add_choice(widget_toggle, hdr_mode_table[i].label);
    }
    gp_widget_set_value(widget_toggle, hdr_label);
    gp_widget_append(section, widget_toggle);

    // Bracketing Mode
    gp_widget_new(GP_WIDGET_RADIO, "Bracketing Mode", &widget_toggle);
    uint32_t bracket_raw = 0;
    ptp_generic_get_uint32(&params, PENTAX_OC_GET_BRACKETING_MODE, &bracket_raw);
    const char *bracket_label = lookup_label(bracketing_mode_table, bracket_raw);
    for (int i = 0; bracketing_mode_table[i].label; i++) {
        gp_widget_add_choice(widget_toggle, bracketing_mode_table[i].label);
    }
    gp_widget_set_value(widget_toggle, bracket_label);
    gp_widget_append(section, widget_toggle);
    gp_widget_append(*window, section);

    // Image Shift Mode
    gp_widget_new(GP_WIDGET_RADIO, "Image Shift Mode", &widget_toggle);
    uint32_t shift_raw = 0;
    ptp_generic_get_uint32(&params, PENTAX_OC_GET_IMAGE_SHIFT, &shift_raw);
    const char *shift_label = lookup_label(shift_mode_table, shift_raw);
    for (int i = 0; shift_mode_table[i].label; i++) {
        gp_widget_add_choice(widget_toggle, shift_mode_table[i].label);
    }
    gp_widget_set_value(widget_toggle, shift_label);
    ptp_generic_get_uint32(&params, PENTAX_OC_GET_IMAGE_SHIFT, &shift_raw);
    enabled = (shift_raw != 0);
    gp_widget_set_value(widget_toggle, &enabled);
    gp_widget_append(section, widget_toggle);
    gp_widget_append(*window, section);

    // Battery Level
    gp_widget_new(GP_WIDGET_TEXT, "Battery Level", &widget_toggle);
    uint32_t batt = 0;
    ptp_generic_get_uint32(&params, PENTAX_OC_GET_BATTERY_LEVEL, &batt);
    static char batt_val[16];
    snprintf(batt_val, sizeof(batt_val), "%u%%", batt);
    gp_widget_set_value(widget_toggle, batt_val);
    gp_widget_append(section, widget_toggle);

    // Exposure Compensation
    gp_widget_new(GP_WIDGET_TEXT, "Exposure Compensation", &widget_toggle);
    uint32_t ec_raw = 0;
    ptp_generic_get_uint32(&params, PENTAX_OC_GET_EXP_COMP, &ec_raw);
    static char ec_val[8];
    snprintf(ec_val, sizeof(ec_val), "%s", lookup_label(exp_comp_table, ec_raw));
    gp_widget_set_value(widget_toggle, ec_val);
    gp_widget_append(section, widget_toggle);

    // Interval Shooting
    gp_widget_new(GP_WIDGET_RANGE, "Interval Time (sec)", &range);
    gp_widget_set_range(range, 1, 3600, 1);
    int default_interval = 5;
    gp_widget_set_value(range, &default_interval);
    gp_widget_append(section, range);

    gp_widget_new(GP_WIDGET_RANGE, "Number of Shots", &range);
    gp_widget_set_range(range, 1, 1000, 1);
    int default_shots = 10;
    gp_widget_set_value(range, &default_shots);
    gp_widget_append(section, range);

    // Mirror Clean Trigger
    gp_widget_new(GP_WIDGET_BUTTON, "Mirror Lock-up for Cleaning", &widget_toggle);
    gp_widget_append(section, widget_toggle);
    gp_widget_append(*window, section);

    gp_widget_new(GP_WIDGET_SECTION, "Focus & Autofocus", &section);
    gp_widget_append(*window, section);

    // Focus Peaking Toggle
    gp_widget_new(GP_WIDGET_TOGGLE, "Focus Peaking Enabled", &widget_toggle);
    uint32_t peaking = 0;
    ptp_generic_get_uint32(&params, PENTAX_OC_GET_PEAKING, &peaking);
    enabled = (peaking != 0);
    gp_widget_set_value(widget_toggle, &enabled);
    gp_widget_append(section, widget_toggle);

    // Face/Eye Detection Toggle
    gp_widget_new(GP_WIDGET_TOGGLE, "Face/Eye Detection Enabled", &widget_toggle);
    uint32_t facedetect = 0;
    ptp_generic_get_uint32(&params, PENTAX_OC_GET_FACE_DETECT, &facedetect);
    enabled = (facedetect != 0);
    gp_widget_set_value(widget_toggle, &enabled);
    gp_widget_append(section, widget_toggle);
    gp_widget_append(*window, section);

    // Live View Zoom Magnification
    gp_widget_new(GP_WIDGET_RADIO, "Live View Zoom", &widget_toggle);
    const char *zoom_choices[] = {"1x", "2x", "4x", "6x", "8x", NULL};
    for (int i = 0; zoom_choices[i]; i++) {
        gp_widget_add_choice(widget_toggle, zoom_choices[i]);
    }
    gp_widget_set_value(widget_toggle, "1x");
    gp_widget_append(section, widget_toggle);
    gp_widget_append(*window, section);
    gp_widget_new(GP_WIDGET_RADIO, "Focus Mode", &widget_toggle);
    gp_widget_append(section, widget_toggle);

    // Fine Focus Buttons
    gp_widget_new(GP_WIDGET_BUTTON, "Focus Fine Tune +1", &widget_toggle);
    gp_widget_append(section, widget_toggle);
    gp_widget_new(GP_WIDGET_BUTTON, "Focus Fine Tune +2", &widget_toggle);
    gp_widget_append(section, widget_toggle);
    gp_widget_new(GP_WIDGET_BUTTON, "Focus Fine Tune -1", &widget_toggle);
    gp_widget_append(section, widget_toggle);
    gp_widget_new(GP_WIDGET_BUTTON, "Focus Fine Tune -2", &widget_toggle);
    gp_widget_append(section, widget_toggle);

    // Zoom Magnification (readonly for now)
    gp_widget_new(GP_WIDGET_TEXT, "Zoom Assist Level", &widget_toggle);
    int zoom_level = 5;
    gp_widget_set_value(widget_toggle, &zoom_level);
    gp_widget_append(section, widget_toggle);
    uint32_t focus_raw = 0;
    ptp_generic_get_uint32(&params, PENTAX_OC_GET_FOCUS_MODE, &focus_raw);
    const char *focus_label = lookup_label(focus_table, focus_raw);
    for (int i = 0; focus_table[i].label; i++) {
        gp_widget_add_choice(widget_toggle, focus_table[i].label);
    }
    gp_widget_set_value(widget_toggle, focus_label);
    gp_widget_append(section, widget_toggle);

    // Continue with hardware section
    gp_widget_new(GP_WIDGET_SECTION, "Storage Settings", &section);
    gp_widget_append(*window, section);

    // Active SD Card Slot
    gp_widget_new(GP_WIDGET_RADIO, "Active SD Card", &widget_toggle);
    gp_widget_add_choice(widget_toggle, "Card 1");
    gp_widget_add_choice(widget_toggle, "Card 2");
    uint32_t active_slot = 0;
    ptp_generic_get_uint32(&params, PENTAX_OC_GET_STORAGE_IDS, &active_slot);
    gp_widget_set_value(widget_toggle, active_slot == 1 ? "Card 1" : "Card 2");
    gp_widget_append(section, widget_toggle);

    // Write Mode
    gp_widget_new(GP_WIDGET_RADIO, "Write Mode", &widget_toggle);
    gp_widget_add_choice(widget_toggle, "Sequential");
    gp_widget_add_choice(widget_toggle, "Parallel");
    gp_widget_add_choice(widget_toggle, "RAW to Card 1, JPEG to Card 2");
    uint32_t write_mode = 0;
    ptp_generic_get_uint32(&params, PENTAX_OC_GET_WRITE_MODE, &write_mode);
    if (write_mode == 0) gp_widget_set_value(widget_toggle, "Sequential");
    else if (write_mode == 1) gp_widget_set_value(widget_toggle, "Parallel");
    else if (write_mode == 2) gp_widget_set_value(widget_toggle, "RAW to Card 1, JPEG to Card 2");
    gp_widget_append(section, widget_toggle);

    gp_widget_new(GP_WIDGET_SECTION, "Hardware Controls", &section);
    gp_widget_append(*window, section);
    gp_widget_new(GP_WIDGET_TOGGLE, "LED On", &widget_toggle);
    int led_on = 0;
    gp_widget_set_value(widget_toggle, &led_on);
    gp_widget_append(section, widget_toggle);
    gp_widget_new(GP_WIDGET_BUTTON, "Trigger GPS Sync", &widget_toggle);
    gp_widget_append(section, widget_toggle);
    gp_widget_append(*window, section);
    gp_widget_new(GP_WIDGET_TEXT, "ISO", &widget_toggle);
    PTPParams params;
    ptp_init(&params, camera->port);
    uint32_t iso_val_raw = 0;
    ptp_generic_get_uint32(&params, PENTAX_OC_GET_ISO, &iso_val_raw);
    static char iso_val[16];
    snprintf(iso_val, sizeof(iso_val), "%s", lookup_label(iso_table, iso_val_raw));
    gp_widget_set_value(widget_toggle, iso_val);
    gp_widget_append(section, widget_toggle);
    gp_widget_new(GP_WIDGET_TEXT, "Shutter Speed", &widget_toggle);
    uint32_t shutter_val_raw = 0;
    ptp_generic_get_uint32(&params, PENTAX_OC_GET_SHUTTER, &shutter_val_raw);
    static char shutter_val[16];
    snprintf(shutter_val, sizeof(shutter_val), "%s", lookup_label(shutter_table, shutter_val_raw));
    gp_widget_set_value(widget_toggle, shutter_val);
    gp_widget_append(section, widget_toggle);
    gp_widget_new(GP_WIDGET_TEXT, "Aperture", &widget_toggle);
    uint32_t aperture_val_raw = 0;
    ptp_generic_get_uint32(&params, PENTAX_OC_GET_APERTURE, &aperture_val_raw);
    static char aperture_val[16];
    snprintf(aperture_val, sizeof(aperture_val), "%s", lookup_label(aperture_table, aperture_val_raw));
    gp_widget_set_value(widget_toggle, aperture_val);
    gp_widget_append(section, widget_toggle);
    gp_widget_new(GP_WIDGET_TEXT, "Drive Mode", &widget_toggle);
    uint32_t drive_val_raw = 0;
    ptp_generic_get_uint32(&params, PENTAX_OC_GET_DRIVE_MODE, &drive_val_raw);
    static char drive_val[16];
    snprintf(drive_val, sizeof(drive_val), "%s", lookup_label(drive_table, drive_val_raw));
    gp_widget_set_value(widget_toggle, drive_val);
    gp_widget_append(section, widget_toggle);

    return GP_OK;
}

static int camera_set_config(Camera *camera, CameraWidget *window, GPContext *context) {
    CameraPrivate *priv = camera->pl;
    CameraWidget *widget;
    int enabled, interval, time_limit, step;
    char *new_val = NULL;

    // Focus Fine Tune
    if (gp_widget_get_child_by_name(window, "Focus Fine Tune +1", &widget) == GP_OK) {
        PTPParams params;
        ptp_init(&params, camera->port);
        ptp_generic_no_data(&params, 0x9524, 1, 0, 0, 0, 0); // +1 step
    }
    if (gp_widget_get_child_by_name(window, "Focus Fine Tune +2", &widget) == GP_OK) {
        PTPParams params;
        ptp_init(&params, camera->port);
        ptp_generic_no_data(&params, 0x9524, 2, 0, 0, 0, 0); // +2 steps
    }
    if (gp_widget_get_child_by_name(window, "Focus Fine Tune -1", &widget) == GP_OK) {
        PTPParams params;
        ptp_init(&params, camera->port);
        ptp_generic_no_data(&params, 0x9524, -1, 0, 0, 0, 0); // -1 step
    }
    if (gp_widget_get_child_by_name(window, "Focus Fine Tune -2", &widget) == GP_OK) {
        PTPParams params;
        ptp_init(&params, camera->port);
        ptp_generic_no_data(&params, 0x9524, -2, 0, 0, 0, 0); // -2 steps
    }


    if (gp_widget_get_child_by_name(window, "Enable Astrotracer", &widget) == GP_OK) {
        gp_widget_get_value(widget, &enabled);
        priv->astrotracer_enabled = enabled;
        PTPParams params;
        ptp_init(&params, camera->port);
        if (enabled)
            pentax_send_simple_command(&params, PENTAX_OC_ENABLE_ASTROTRACER);
        else
            pentax_send_simple_command(&params, PENTAX_OC_DISABLE_ASTROTRACER);
    }
    if (gp_widget_get_child_by_name(window, "Astrotracer Time Limit (sec)", &widget) == GP_OK) {
        gp_widget_get_value(widget, &time_limit);
        priv->astrotracer_time_limit = time_limit;
        PTPParams params;
        ptp_init(&params, camera->port);
        pentax_set_astrotracer_time(&params, time_limit);
    }
    if (gp_widget_get_child_by_name(window, "Bulb Mode Active", &widget) == GP_OK) {
        gp_widget_get_value(widget, &enabled);
        priv->bulb_mode_active = enabled;
        pentax_set_mirror_up(camera, enabled);
    }
    if (gp_widget_get_child_by_name(window, "Live View Active", &widget) == GP_OK) {
        gp_widget_get_value(widget, &enabled);
        if (enabled && !priv->live_view_active) {
            PTPParams params;
            ptp_init(&params, camera->port);
            pentax_start_live_view(&params);
            priv->live_view_active = TRUE;
        } else if (!enabled && priv->live_view_active) {
            PTPParams params;
            ptp_init(&params, camera->port);
            pentax_end_live_view(&params);
            priv->live_view_active = FALSE;
        }
    }
    if (gp_widget_get_child_by_name(window, "Live View Interval (ms)", &widget) == GP_OK) {
        gp_widget_get_value(widget, &interval);
        priv->live_view_interval = interval;
    }
    if (gp_widget_get_child_by_name(window, "ISO", &widget) == GP_OK) {
        gp_widget_get_value(widget, &new_val);
        PTPParams params;
        ptp_init(&params, camera->port);
        uint32_t iso = lookup_value(iso_table, new_val);
        ptp_generic_no_data(&params, PENTAX_OC_SET_ISO, iso, 0, 0, 0, 0);
    }
    if (gp_widget_get_child_by_name(window, "Shutter Speed", &widget) == GP_OK) {
        gp_widget_get_value(widget, &new_val);
        PTPParams params;
        ptp_init(&params, camera->port);
        uint32_t shutter = lookup_value(shutter_table, new_val); // Map string to value via lookup in future
        ptp_generic_no_data(&params, PENTAX_OC_SET_SHUTTER, shutter, 0, 0, 0, 0);
    }
    if (gp_widget_get_child_by_name(window, "Aperture", &widget) == GP_OK) {
        gp_widget_get_value(widget, &new_val);
        PTPParams params;
        ptp_init(&params, camera->port);
        uint32_t aperture = lookup_value(aperture_table, new_val); // Map string to value via lookup in future
        ptp_generic_no_data(&params, PENTAX_OC_SET_APERTURE, aperture, 0, 0, 0, 0);
    }
    if (gp_widget_get_child_by_name(window, "Drive Mode", &widget) == GP_OK) {
        gp_widget_get_value(widget, &new_val);
        PTPParams params;
        ptp_init(&params, camera->port);
        uint32_t drive = lookup_value(drive_table, new_val);
        ptp_generic_no_data(&params, PENTAX_OC_SET_DRIVE_MODE, drive, 0, 0, 0, 0);
    }
    if (gp_widget_get_child_by_name(window, "Exposure Mode", &widget) == GP_OK) {
        gp_widget_get_value(widget, &new_val);
        PTPParams params;
        ptp_init(&params, camera->port);
        uint32_t mode = lookup_value(exposure_mode_table, new_val);
        ptp_generic_no_data(&params, 0x9540, mode, 0, 0, 0, 0);
    }
    if (gp_widget_get_child_by_name(window, "Scene Mode", &widget) == GP_OK) {
        gp_widget_get_value(widget, &new_val);
        PTPParams params;
        ptp_init(&params, camera->port);
        uint32_t mode = lookup_value(scene_mode_table, new_val);
        ptp_generic_no_data(&params, 0x9541, mode, 0, 0, 0, 0);
    }
    if (gp_widget_get_child_by_name(window, "Image Tone", &widget) == GP_OK) {
        gp_widget_get_value(widget, &new_val);
        PTPParams params;
        ptp_init(&params, camera->port);
        uint32_t tone = lookup_value(image_tone_table, new_val);
        ptp_generic_no_data(&params, 0x9542, tone, 0, 0, 0, 0);
    }
    if (gp_widget_get_child_by_name(window, "CI Mode", &widget) == GP_OK) {
        gp_widget_get_value(widget, &new_val);
        PTPParams params;
        ptp_init(&params, camera->port);
        uint32_t ci = lookup_value(ci_mode_table, new_val);
        ptp_generic_no_data(&params, 0x9543, ci, 0, 0, 0, 0);
    }

    return GP_OK;
}

static int camera_trigger_autofocus(Camera *camera, GPContext *context) {
    CameraPrivate *priv = camera->pl;
    PTPParams params;
    ptp_init(&params, camera->port);
    int ret = ptp_generic_no_data(&params, PENTAX_OC_AF_TRIGGER, 0, 0, 0, 0, 0);
    return (ret == PTP_RC_OK) ? GP_OK : GP_ERROR;
}

static CameraOperations camera_ops = {
    .init = camera_init,
    .exit = camera_exit,
    .capture = camera_capture,
    .get_config = camera_get_config,
    .set_config = camera_set_config,
    .get_preview = camera_get_preview,
    .trigger_autofocus = camera_trigger_autofocus,
    .file_list_func = camera_list_files,
    .get_file_func = camera_get_file,
    .delete_file_func = camera_delete_file
};

int camera_id(CameraText *id) {
    strcpy(id->text, "Pentax K-series Enhanced Driver");
    return GP_OK;
}

int camera_abilities(CameraAbilitiesList *list) {
    CameraAbilities a;
    memset(&a, 0, sizeof(CameraAbilities));

    strcpy(a.model, "Pentax K-1 II");
    a.status = GP_DRIVER_STATUS_EXPERIMENTAL;
    a.port = GP_PORT_USB;
    a.operations = GP_OPERATION_CAPTURE_IMAGE | GP_OPERATION_CONFIG | GP_OPERATION_CAPTURE_PREVIEW;
    gp_abilities_list_append(list, a);

    strcpy(a.model, "Pentax K-3 III");
    gp_abilities_list_append(list, a);

    strcpy(a.model, "Pentax K-70");
    gp_abilities_list_append(list, a);

    return GP_OK;
}

#define PENTAX_OC_GET_OBJECT_HANDLES 0x9801
#define PENTAX_OC_GET_OBJECT_INFO    0x9802
#define PENTAX_OC_GET_OBJECT         0x9803
#define PENTAX_OC_DELETE_OBJECT      0x9804

static int camera_list_files(Camera *camera, const char *folder, CameraList *list, GPContext *context) {
    PTPParams params;
    ptp_init(&params, camera->port);

    CameraPrivate *priv = camera->pl;
    uint32_t active_storage = 0;
    ptp_generic_get_uint32(&params, PENTAX_OC_GET_STORAGE_IDS, &active_storage);

    uint32_t *handles = NULL;
    unsigned int n;
    int ret = ptp_getobjecthandles(&params, active_storage, 0x000000, 0x000000, &handles, &n);
    if (ret != PTP_RC_OK) return GP_ERROR;

    for (unsigned int i = 0; i < n; i++) {
        PTPObjectInfo oi;
        ret = ptp_getobjectinfo(&params, handles[i], &oi);
        if (ret == PTP_RC_OK) {
            gp_list_append(list, oi.Filename, NULL);
        }
    }

    free(handles);
    return GP_OK;
}

static int camera_get_file(Camera *camera, const char *folder, const char *filename, GPFileType type, CameraFile *file, GPContext *context) {
    PTPParams params;
    ptp_init(&params, camera->port);

    uint32_t *handles = NULL;
    unsigned int n;
    int ret = ptp_getobjecthandles(&params, 0xFFFFFFFF, 0x000000, 0x000000, &handles, &n);
    if (ret != PTP_RC_OK) return GP_ERROR;

    for (unsigned int i = 0; i < n; i++) {
        PTPObjectInfo oi;
        ret = ptp_getobjectinfo(&params, handles[i], &oi);
        if (ret == PTP_RC_OK && strcmp(oi.Filename, filename) == 0) {
            char *data = NULL;
            unsigned long size = 0;
            ret = ptp_getobject(&params, handles[i], &data, &size);
            if (ret == PTP_RC_OK && data && size > 0) {
                gp_file_set_data_and_size(file, data, size);
                free(handles);
                return GP_OK;
            }
        }
    }

    free(handles);
    return GP_ERROR;
}

static int camera_delete_file(Camera *camera, const char *folder, const char *filename, GPContext *context) {
    PTPParams params;
    ptp_init(&params, camera->port);

    uint32_t *handles = NULL;
    unsigned int n;
    int ret = ptp_getobjecthandles(&params, 0xFFFFFFFF, 0x000000, 0x000000, &handles, &n);
    if (ret != PTP_RC_OK) return GP_ERROR;

    for (unsigned int i = 0; i < n; i++) {
        PTPObjectInfo oi;
        ret = ptp_getobjectinfo(&params, handles[i], &oi);
        if (ret == PTP_RC_OK && strcmp(oi.Filename, filename) == 0) {
            ret = ptp_deleteobject(&params, handles[i]);
            free(handles);
            return (ret == PTP_RC_OK) ? GP_OK : GP_ERROR;
        }
    }

    free(handles);
    return GP_ERROR;
}

CameraLibrary camera_library = {
    .id = camera_id,
    .abilities = camera_abilities,
    .operations = &camera_ops
};
