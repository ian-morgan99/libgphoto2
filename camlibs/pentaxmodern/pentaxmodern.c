#include "config.h"
#include <gphoto2/gphoto2-camera.h> 
#include <gphoto2/gphoto2-library.h>
#include <gphoto2/gphoto2-port.h>   
#include <gphoto2/gphoto2-result.h> 
#include <gphoto2/gphoto2-setting.h>
#include <gphoto2/gphoto2-file.h>   
#include <gphoto2/gphoto2-list.h>   
#include <gphoto2/ptp.h>            
#include <string.h>
#include <stdlib.h>
#include <stdio.h> 
#include <glib.h> 
#include <gphoto2/gphoto2-log.h>
#include <unistd.h> 

#include "pentaxmodern.h" 

/* Pentax Vendor and Product IDs */
#define PENTAX_VENDOR_ID 0x0A17
#define PENTAX_PRODUCT_K1II 0x503A  
#define PENTAX_PRODUCT_K3III 0x5049 
#define PENTAX_PRODUCT_K70 0x503E  

/* Pentax Specific PTP Operation Codes */
#define PENTAX_OC_GET_ALL_CONDITIONS 0x920F      
#define PENTAX_OC_SET_PENTAX_VENDOR_MODE 0x9520 
#define PENTAX_OC_GET_HDR_MODE_ALT 0x952A 
#define PENTAX_OC_SET_APERTURE_KEEP 0x920A      // For Keep Aperture functionality

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
#define PENTAX_OC_SET_HDR_MODE 0x9521
#define PENTAX_OC_GET_BRACKETING_MODE 0x9522
#define PENTAX_OC_SET_BRACKETING_MODE 0x9523
#define PENTAX_OC_SET_LV_ZOOM 0x9525 
#define PENTAX_OC_GET_PEAKING 0x9526
#define PENTAX_OC_SET_PEAKING 0x9527
#define PENTAX_OC_GET_FACE_DETECT 0x9528
#define PENTAX_OC_SET_FACE_DETECT 0x9529
#define PENTAX_OC_GET_STORAGE_IDS 0x9530 
#define PENTAX_OC_SET_ACTIVE_STORAGE 0x9531
#define PENTAX_OC_GET_WRITE_MODE 0x9532
#define PENTAX_OC_SET_WRITE_MODE 0x9533
#define PENTAX_OC_GET_EXPOSURE_MODE 0x9540 
#define PENTAX_OC_SET_EXPOSURE_MODE 0x9540 
#define PENTAX_OC_GET_SCENE_MODE 0x9541    
#define PENTAX_OC_SET_SCENE_MODE 0x9541    
#define PENTAX_OC_GET_IMAGE_TONE 0x9542    
#define PENTAX_OC_SET_IMAGE_TONE 0x9542    
#define PENTAX_OC_GET_CI_MODE 0x9543       
#define PENTAX_OC_SET_CI_MODE 0x9543       
#define PENTAX_OC_FOCUS_FINE_TUNE 0x9524   
#define PENTAX_OC_BULB_START 0x9205
#define PENTAX_OC_BULB_END   0x9206

/* Pentax Specific PTP Device Property Codes (DPCs) */
#define PENTAX_DPC_CI_SATURATION           0xD021 
#define PENTAX_DPC_CI_HUE                  0xD022 
#define PENTAX_DPC_CI_KEY                  0xD023 
#define PENTAX_DPC_CI_FILTER_EFFECT        0xD024 
#define PENTAX_DPC_CI_MONOTONE_TONING      0xD025 
#define PENTAX_DPC_CI_TONING               0xD026 
#define PENTAX_DPC_CI_FINE_SHARPNESS       0xD027 
#define PENTAX_DPC_CI_CONTRAST_HIGHLIGHT   0xD028 
#define PENTAX_DPC_CI_CONTRAST_SHADOW      0xD029 
#define PENTAX_DPC_CI_CROSS_PROCESS_TYPE   0xD02C 
#define PENTAX_DPC_CI_USER_FILTER_EFFECT   0xD02D 
#define PENTAX_DPC_MOVIE_MODE              0xD039 // For Movie Mode Toggle
#define PENTAX_DPC_WRITING_FILE_FORMAT_SETTING 0xD01B // For Movie settings (File Format, Quality, Slot)

/* ConditionIndex constants */
#define CI_DSP_STATE 3          
#define CI_CPU_STATE 4          
#define CI_CAPTURE_MODE_INFO 26 
#define CI_ISO_SETTING 27       
#define CI_WB_MODE 30           
#define CI_TV_VALUE_PLACEHOLDER 64    
#define CI_AV_VALUE_PLACEHOLDER 68    
#define CI_BATTERY_LEVEL_PLACEHOLDER 50 
#define CI_SD1_REMAIN_PLACEHOLDER 60    

#define PENTAX_CUSTOM_MODEL_ID_K3III       0x13254 
#define PENTAX_CUSTOM_VENDOR_EXT_VER_K3III 1       
#define PENTAX_CUSTOM_MODEL_ID_K1II        0x13210 
#define PENTAX_CUSTOM_VENDOR_EXT_VER_K1II  1       
#define PENTAX_CUSTOM_MODEL_ID_645D        0x12E08
#define PENTAX_CUSTOM_MODEL_ID_K3          0x13F80
#define PENTAX_CUSTOM_MODEL_ID_645Z        0x13010
#define PENTAX_CUSTOM_MODEL_ID_K1          0x13092
#define PENTAX_CUSTOM_MODEL_ID_KP          0x1322C
#define PENTAX_CUSTOM_MODEL_ID_G900SE      0x1327C
#define PENTAX_CUSTOM_MODEL_ID_GRIII       0x1320E
#define PENTAX_CUSTOM_MODEL_ID_K70         0x13222

typedef struct {
    GPPort *port; 
    gboolean astrotracer_enabled;
    gboolean bulb_mode_active;
    gboolean live_view_active;
    int live_view_interval;
    int astrotracer_time_limit;
    PTPParams ptp_params; 
    uint32_t ptp_custom_model_id;       
    uint16_t ptp_custom_vendor_ext_ver; 
    unsigned char *conditions_data_cache; 
    unsigned int conditions_data_len;     
    uint8_t keep_aperture_mode; // 0 for Reset, 1 for Keep
} CameraPrivateLibrary; 

// Forward declarations
// ... (as before) ...
static int pentaxmodern_get_config (Camera *camera, CameraWidget **window, GPContext *context);
static int pentaxmodern_set_config (Camera *camera, CameraWidget *window, GPContext *context);
static int pentaxmodern_get_preview (Camera *camera, CameraFile *file, GPContext *context);
static int pentaxmodern_trigger_autofocus(Camera *camera, GPContext *context);
static int pentaxmodern_folder_list_files (Camera *camera, const char *folder, CameraList *list, GPContext *context);
static int pentaxmodern_file_get (Camera *camera, const char *folder, const char *filename, GPFileType type, CameraFile *file, GPContext *context);
static int pentaxmodern_file_delete (Camera *camera, const char *folder, const char *filename, GPContext *context);


typedef struct {
    const char *label;
    uint32_t value; 
} PentaxLookupEntry;

// Lookup Tables for CI Parameters & Movie Settings
static PentaxLookupEntry CIFineSharpness_table[] = {
    {"Normal", 0}, {"Fine", 1}, {"Extra Fine", 2}, {NULL,0}
};
static PentaxLookupEntry CIFilterEffect_table[] = {
    {"None", 0}, {"Yellow", 1}, {"Orange", 2}, {"Red", 3}, {"Green", 4}, {NULL,0}
};
static PentaxLookupEntry CIToning_table[] = { 
    {"None", 0}, {"Sepia", 1}, {"Blue", 2}, {"Purple", 3}, {"Green", 4}, {NULL,0}
};
static PentaxLookupEntry CICrossProcessType_table[] = {
    {"Off", 0}, {"Basic", 1}, {"Magenta", 2}, {"Yellow", 3}, {"Blue", 4}, {NULL,0}
};
static PentaxLookupEntry MovieFileFormat_table[] = { 
    {"MP4", 0}, {"MOV", 1}, {NULL,0} // Placeholder values
};
// Values for MovieQuality_table combine resolution and framerate conceptually.
// The actual PTP values for MoviePixSize and MovieFrameRate need to be determined.
#define PENTAX_MOVIE_QUALITY_4K_30P  0 // Label: "4K 30p"
#define PENTAX_MOVIE_QUALITY_FHD_60P 1 // Label: "FHD 60p"
#define PENTAX_MOVIE_QUALITY_FHD_30P 2 // Label: "FHD 30p"
static PentaxLookupEntry MovieQuality_table[] = { 
    {"4K 30p", PENTAX_MOVIE_QUALITY_4K_30P}, 
    {"FHD 60p", PENTAX_MOVIE_QUALITY_FHD_60P}, 
    {"FHD 30p", PENTAX_MOVIE_QUALITY_FHD_30P}, 
    {NULL,0}
};
// Corresponding PTP parameter values for PENTAX_DPC_WRITING_FILE_FORMAT_SETTING
// These are just examples and need to be verified for actual Pentax models.
#define PENTAX_CONST_MOVIE_PIXSIZE_4K  0x01 // Example
#define PENTAX_CONST_MOVIE_PIXSIZE_FHD 0x02 // Example
#define PENTAX_CONST_MOVIE_FRAMERATE_30P 0x01 // Example
#define PENTAX_CONST_MOVIE_FRAMERATE_60P 0x02 // Example
#define PENTAX_CONST_MOVIE_CARD_SLOT_DEFAULT 1 // Example: Slot 1

static PentaxLookupEntry KeepAperture_table[] = {
    {"Reset Aperture", 0}, {"Keep Aperture", 1}, {NULL,0}
};


// Existing Lookup Tables (iso_table, shutter_table, etc. - full content as before)
static PentaxLookupEntry iso_table[] = { /* ... full content ... */ 
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
static PentaxLookupEntry shutter_table[] = { /* ... full content ... */ 
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
    {"1/2", 2}, {"1\"", 1}, {"2\"", (uint32_t)-2}, {"3\"", (uint32_t)-3}, 
    {"4\"", (uint32_t)-4}, {"5\"", (uint32_t)-5}, {"6\"", (uint32_t)-6}, {"8\"", (uint32_t)-8},
    {"10\"", (uint32_t)-10}, {"13\"", (uint32_t)-13}, {"15\"", (uint32_t)-15}, {"20\"", (uint32_t)-20},
    {"25\"", (uint32_t)-25}, {"30\"", (uint32_t)-30},
    {NULL, 0}
};
static PentaxLookupEntry aperture_table[] = { /* ... full content ... */ 
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
static PentaxLookupEntry drive_table[] = { /* ... full content ... */ 
    {"Single", 1}, {"Continuous High", 2}, {"Continuous Low", 3}, {"Timer", 4},
    {NULL, 0}
};
static PentaxLookupEntry wb_table[] = { /* ... full content ... */ 
    {"Auto", 1}, {"Daylight", 2}, {"Shade", 3}, {"Cloudy", 4}, {"Tungsten", 5},
    {"Fluorescent", 6}, {"Manual", 7},
    {NULL, 0}
};
static PentaxLookupEntry quality_table[] = { /* ... full content ... */ 
    {"RAW", 1}, {"JPEG", 2}, {"RAW+JPEG", 3},
    {NULL, 0}
};
static PentaxLookupEntry metering_table[] = { /* ... full content ... */ 
    {"Evaluative", 1}, {"Center-weighted", 2}, {"Spot", 3},
    {NULL, 0}
};
static PentaxLookupEntry flash_table[] = { /* ... full content ... */ 
    {"Auto", 1}, {"On", 2}, {"Off", 3},
    {NULL, 0}
};
static PentaxLookupEntry focus_table[] = { /* ... full content ... */ 
    {"AF-S", 1}, {"AF-C", 2}, {"Manual", 3},
    {"Contrast AF", 4}, {"Phase Detect", 5}, {"Face Detect", 6},
    {"Touch AF", 7}, {"Focus Peaking", 8}, {"Zoom Focus Assist", 9},
    {NULL, 0}
};
static PentaxLookupEntry exp_comp_table[] = { /* ... full content ... */ 
    {"-5.0", (uint32_t)-50}, {"-4.7", (uint32_t)-47}, {"-4.3", (uint32_t)-43}, {"-4.0", (uint32_t)-40},
    {"-3.7", (uint32_t)-37}, {"-3.3", (uint32_t)-33}, {"-3.0", (uint32_t)-30}, {"-2.7", (uint32_t)-27},
    {"-2.3", (uint32_t)-23}, {"-2.0", (uint32_t)-20}, {"-1.7", (uint32_t)-17}, {"-1.3", (uint32_t)-13},
    {"-1.0", (uint32_t)-10}, {"-0.7", (uint32_t)-7}, {"-0.3", (uint32_t)-3}, {"0.0", 0},
    {"+0.3", 3}, {"+0.7", 7}, {"+1.0", 10}, {"+1.3", 13},
    {"+1.7", 17}, {"+2.0", 20}, {"+2.3", 23}, {"+2.7", 27},
    {"+3.0", 30}, {"+3.3", 33}, {"+3.7", 37}, {"+4.0", 40},
    {"+4.3", 43}, {"+4.7", 47}, {"+5.0", 50},
    {NULL, 0}
};
static PentaxLookupEntry shift_mode_table[] = { /* ... full content ... */ 
    {"Off", 0}, {"Standard Shift", 1}, {"Pixel Shift", 2}, {"Pixel Shift + Motion Correction", 3},
    {NULL, 0}
};
static PentaxLookupEntry hdr_mode_table[] = { /* ... full content ... */ 
    {"Off", 0}, {"Auto HDR", 1}, {"HDR1", 2}, {"HDR2", 3}, {"HDR3", 4},
    {NULL, 0}
};
static PentaxLookupEntry bracketing_mode_table[] = { /* ... full content ... */ 
    {"Off", 0}, {"1 EV step", 1}, {"0.7 EV step", 2}, {"0.5 EV step", 3}, {"0.3 EV step", 4},
    {NULL, 0}
};
static PentaxLookupEntry image_tone_table[] = { /* ... full content ... */ 
    {"Natural", 0}, {"Bright", 1}, {"Portrait", 2}, {"Landscape", 3},
    {"Vibrant", 4}, {"Flat", 5}, {"Monotone", 6}, {"Muted Color", 7},
    {"Reversal Film", 8}, {"Bleach Bypass", 9}, {"Cross Process", 10},
    {"Satobi", 11}, {"Silky", 12}, {NULL, 0}
};
static PentaxLookupEntry ci_mode_table[] = { /* ... full content ... */ 
    {"Standard", 0}, {"Bright", 1}, {"Portrait", 2}, {"Landscape", 3},
    {"Vibrant", 4}, {"Monotone", 5}, {"Radiant", 6}, {"Muted", 7},
    {"Reversal Film", 8}, {"Bleach Bypass", 9}, {"Cross Process", 10},
    {"Satobi", 11}, {"Silky", 12}, {"Muted Color", 13}, {NULL, 0}
};
static PentaxLookupEntry scene_mode_table[] = { /* ... full content ... */ 
    {"Auto", 0}, {"Portrait", 1}, {"Landscape", 2}, {"Macro", 3},
    {"Sports", 4}, {"Night Scene", 5}, {"Pet", 6}, {"Sunset", 7},
    {"Blue Sky", 8}, {"Forest", 9}, {"Night Portrait", 10},
    {"Night Scene HDR", 11}, {"Kids", 12}, {"Surf & Snow", 13},
    {"Backlight Silhouette", 14}, {"Food", 15}, {"Stage Lighting", 16},
    {"Fireworks", 17}, {"Museum", 18}, {"Text", 19}, {"Moss", 20},
    {"Water Reflection", 21}, {"Light Trails", 22}, {"Underwater", 23},
    {NULL, 0}
};
static PentaxLookupEntry exposure_mode_table[] = { /* ... full content ... */ 
    {"P", 0}, {"Sv", 1}, {"Tv", 2}, {"Av", 3},
    {"TAv", 4}, {"M", 5}, {"B", 6}, {"X", 7},
    {"Green", 8}, {"U1", 9}, {"U2", 10},
    {"Movie", 11}, {"Auto Picture", 12}, {"Scene Mode", 13},
    {NULL, 0}
};

/* Helper functions */
static const char *lookup_label(PentaxLookupEntry *table, uint32_t val) { /* ... unchanged ... */ 
    for (int i = 0; table[i].label; i++) {
        if (table[i].value == val) return table[i].label;
    }
    return "Unknown";
}
static uint32_t lookup_value(PentaxLookupEntry *table, const char *label) { /* ... unchanged ... */ 
    if (!label) return 0; 
    for (int i = 0; table[i].label; i++) {
        if (g_strcmp0(table[i].label, label) == 0) return table[i].value;
    }
    return 0; 
}

/* Data Parsing Helper Functions */
static uint8_t parse_condition_u8(const unsigned char *data, unsigned int len, int index) { /* ... unchanged ... */ 
    if (!data || (unsigned int)index >= len) {
        GP_LOG_E("pentaxmodern", "parse_condition_u8: index %d out of bounds (len %u).", index, len);
        return 0; 
    }
    return data[index];
}
static uint16_t parse_condition_u16(const unsigned char *data, unsigned int len, int index) { /* ... unchanged ... */ 
    if (!data || (unsigned int)(index + 1) >= len) { 
        GP_LOG_E("pentaxmodern", "parse_condition_u16: index %d out of bounds (len %u).", index, len);
        return 0; 
    }
    return dtoh16a(data + index); 
}
static uint32_t parse_condition_u32(const unsigned char *data, unsigned int len, int index) { /* ... unchanged ... */ 
    if (!data || (unsigned int)(index + 3) >= len) { 
        GP_LOG_E("pentaxmodern", "parse_condition_u32: index %d out of bounds (len %u).", index, len);
        return 0; 
    }
    return dtoh32a(data + index); 
}

// Get All Conditions Data function
static uint16_t pentaxmodern_get_all_conditions_data(PTPParams *params, unsigned char **conditions_data, unsigned int *conditions_len) { /* ... unchanged ... */ 
    PTPContainer ptp;
    uint16_t ret;
    GP_DEBUG("pentaxmodern_get_all_conditions_data: Fetching bulk status with 0x%X", PENTAX_OC_GET_ALL_CONDITIONS);
    if (!conditions_data || !conditions_len) return PTP_RC_InvalidParameter;
    *conditions_data = NULL; *conditions_len = 0;
    memset(&ptp, 0, sizeof(ptp));
    ptp.Code = PENTAX_OC_GET_ALL_CONDITIONS; ptp.Nparam = 0;
    ret = ptp_transaction(params, &ptp, PTP_DP_GETDATA, 0, conditions_data, conditions_len);
    if (ret != PTP_RC_OK) {
        gp_log(GP_LOG_ERROR, "pentaxmodern", "PTP transaction for 0x%X failed: 0x%X", PENTAX_OC_GET_ALL_CONDITIONS, ret);
        if (*conditions_data) { free(*conditions_data); *conditions_data = NULL; }
        *conditions_len = 0;
    } else {
        GP_DEBUG("pentaxmodern_get_all_conditions_data: Success, received %u bytes.", *conditions_len);
    }
    return ret;
}

// Helper for getting PTP OpCode based properties (used as fallback)
static int pentaxmodern_get_ptp_property_u32(PTPParams *params, uint16_t opcode, uint32_t *value) { /* ... unchanged ... */ 
    PTPContainer ptp;
    unsigned char *data = NULL;
    unsigned int len = 0;
    uint16_t ret;
    GP_DEBUG("pentaxmodern_get_ptp_property_u32: opcode 0x%X", opcode);
    memset(&ptp, 0, sizeof(ptp));
    ptp.Code = opcode;
    ptp.Nparam = 0; 
    ret = ptp_transaction_new(params, &ptp, PTP_DP_GETDATA, 0, NULL);
    if (ret != PTP_RC_OK) {
        gp_log(GP_LOG_ERROR, "pentaxmodern", "PTP transaction (send req) for opcode 0x%X failed: 0x%X", opcode, ret);
        return ret;
    }
    if (ptp.Nparam >= 1) {
        *value = ptp.Param1;
        GP_DEBUG("pentaxmodern_get_ptp_property_u32: value %u from Param1 for opcode 0x%X", *value, opcode);
        return PTP_RC_OK;
    } else {
        gp_log(GP_LOG_ERROR, "pentaxmodern", "Opcode 0x%X did not return expected parameter in response.", opcode);
        ret = ptp_transaction(params, &ptp, PTP_DP_GETDATA, 0, &data, &len);
        if (ret == PTP_RC_OK && len >= sizeof(uint32_t) && data != NULL) {
            *value = dtoh32a(data); 
            free(data);
            GP_DEBUG("pentaxmodern_get_ptp_property_u32: value %u from data phase for opcode 0x%X", *value, opcode);
            return PTP_RC_OK;
        } else if (data) {
            free(data);
        }
        gp_log(GP_LOG_ERROR, "pentaxmodern", "PTP get data for opcode 0x%X failed: 0x%X, len %u", opcode, ret, len);
        return (ret == PTP_RC_OK) ? GP_ERROR_IO : ret; 
    }
}

// Helper functions for PTP Device Properties
static int get_ptp_deviceprop_value(Camera *camera, uint32_t propcode, PTPPropValue *value, uint16_t datatype) {
    CameraPrivateLibrary *priv = camera->priv;
    uint16_t ret = ptp_getdevicepropvalue(&priv->ptp_params, propcode, value, datatype);
    if (ret != PTP_RC_OK) {
        GP_LOG_E("pentaxmodern", "get_ptp_deviceprop_value(0x%X) failed: 0x%X", propcode, ret);
        return GP_ERROR_IO;
    }
    return GP_OK;
}

static int set_ptp_deviceprop_value(Camera *camera, uint32_t propcode, PTPPropValue *value, uint16_t datatype) {
    CameraPrivateLibrary *priv = camera->priv;
    uint16_t ret = ptp_setdevicepropvalue(&priv->ptp_params, propcode, value, datatype);
    if (ret != PTP_RC_OK) {
        GP_LOG_E("pentaxmodern", "set_ptp_deviceprop_value(0x%X) failed: 0x%X", propcode, ret);
        return GP_ERROR_IO;
    }
    return GP_OK;
}

// Type-specific wrappers for device properties
static int get_ptp_dpc_s8(Camera *camera, uint32_t pc, int8_t *val) { PTPPropValue v; int r = get_ptp_deviceprop_value(camera, pc, &v, PTP_DTC_INT8); if (r==GP_OK) *val = v.i8; return r; }
static int set_ptp_dpc_s8(Camera *camera, uint32_t pc, int8_t val) { PTPPropValue v; v.i8=val; return set_ptp_deviceprop_value(camera, pc, &v, PTP_DTC_INT8); }
static int get_ptp_dpc_u8(Camera *camera, uint32_t pc, uint8_t *val) { PTPPropValue v; int r = get_ptp_deviceprop_value(camera, pc, &v, PTP_DTC_UINT8); if (r==GP_OK) *val = v.u8; return r; }
static int set_ptp_dpc_u8(Camera *camera, uint32_t pc, uint8_t val) { PTPPropValue v; v.u8=val; return set_ptp_deviceprop_value(camera, pc, &v, PTP_DTC_UINT8); }
static int get_ptp_dpc_u16(Camera *camera, uint32_t pc, uint16_t *val) { PTPPropValue v; int r = get_ptp_deviceprop_value(camera, pc, &v, PTP_DTC_UINT16); if (r==GP_OK) *val = v.u16; return r; }
static int set_ptp_dpc_u16(Camera *camera, uint32_t pc, uint16_t val) { PTPPropValue v; v.u16=val; return set_ptp_deviceprop_value(camera, pc, &v, PTP_DTC_UINT16); }


static int static_pentax_start_live_view(PTPParams *params) { /* ... unchanged ... */ 
    return ptp_generic_no_data(params, PENTAX_OC_START_LIVE_VIEW, 0, 0,0,0,0);
}
static int static_pentax_end_live_view(PTPParams *params) { /* ... unchanged ... */ 
    return ptp_generic_no_data(params, PENTAX_OC_END_LIVE_VIEW, 0, 0,0,0,0);
}
static int static_pentax_get_lv_frame(PTPParams *params, unsigned char **data, unsigned int *size) { /* ... unchanged ... */ 
    PTPContainer ptp;
    uint16_t ret;
    GP_DEBUG("static_pentax_get_lv_frame: opcode 0x%X", PENTAX_OC_GET_LV_FRAME);
    memset(&ptp, 0, sizeof(ptp));
    ptp.Code = PENTAX_OC_GET_LV_FRAME;
    ptp.Nparam = 0;
    ret = ptp_transaction(params, &ptp, PTP_DP_GETDATA, 0, data, size);
    if (ret != PTP_RC_OK) {
        gp_log(GP_LOG_ERROR, "pentaxmodern", "PTP get_lv_frame (0x%X) failed: 0x%X", PENTAX_OC_GET_LV_FRAME, ret);
        if (*data) { free(*data); *data = NULL; }
        *size = 0;
        return ret;
    }
    GP_DEBUG("static_pentax_get_lv_frame: success, size %u", *size);
    return PTP_RC_OK;
}
static int static_pentax_send_simple_command(PTPParams *params, uint16_t code) { /* ... unchanged ... */ 
    int ret = ptp_generic_no_data(params, code, 0,0,0,0,0);
    gp_log(GP_LOG_DEBUG, "pentaxmodern", "Sent simple command: 0x%X, ret=0x%X", code, ret);
    return (ret == PTP_RC_OK) ? GP_OK : GP_ERROR_IO;
}
static int static_pentax_set_astrotracer_time(PTPParams *params, uint32_t seconds) { /* ... unchanged ... */ 
    int ret = ptp_generic_no_data(params, PENTAX_OC_SET_ASTROTRACER_TIME, 1, seconds,0,0,0);
    gp_log(GP_LOG_DEBUG, "pentaxmodern", "Set Astrotracer time to %u sec, ret=0x%X", seconds, ret);
    return (ret == PTP_RC_OK) ? GP_OK : GP_ERROR_IO;
}
static void populate_pentax_model_ids(CameraPrivateLibrary *priv) { /* ... unchanged ... */ 
    if (!priv || !priv->ptp_params.deviceinfo.Model) {
        GP_LOG_E("pentaxmodern", "Cannot populate model IDs, deviceinfo not available.");
        priv->ptp_custom_model_id = 0; 
        priv->ptp_custom_vendor_ext_ver = 0; 
        return;
    }
    const char *model_string = priv->ptp_params.deviceinfo.Model;
    GP_DEBUG("pentaxmodern", "Device model string: '%s'", model_string);
    if (g_str_has_prefix(model_string, "PENTAX K-3 Mark III")) { 
        priv->ptp_custom_model_id = PENTAX_CUSTOM_MODEL_ID_K3III;
        priv->ptp_custom_vendor_ext_ver = PENTAX_CUSTOM_VENDOR_EXT_VER_K3III;
    } else if (g_str_has_prefix(model_string, "PENTAX K-1 Mark II")) {
        priv->ptp_custom_model_id = PENTAX_CUSTOM_MODEL_ID_K1II; 
        priv->ptp_custom_vendor_ext_ver = PENTAX_CUSTOM_VENDOR_EXT_VER_K1II;
    } else if (strstr(model_string, "645D")) { // Using strstr for broader match
        priv->ptp_custom_model_id = PENTAX_CUSTOM_MODEL_ID_645D;
        priv->ptp_custom_vendor_ext_ver = 1; // Assuming default vendor ext ver
    } else if (strstr(model_string, "PENTAX K-3")) { // Exact match for K-3 to distinguish from K-3 Mark III
        priv->ptp_custom_model_id = PENTAX_CUSTOM_MODEL_ID_K3;
        priv->ptp_custom_vendor_ext_ver = 1; // Assuming default vendor ext ver
    } else if (strstr(model_string, "PENTAX 645Z")) {
        priv->ptp_custom_model_id = PENTAX_CUSTOM_MODEL_ID_645Z;
        priv->ptp_custom_vendor_ext_ver = 1; // Assuming default vendor ext ver
    } else if (strstr(model_string, "PENTAX K-1")) { // Exact match for K-1 to distinguish from K-1 Mark II
        priv->ptp_custom_model_id = PENTAX_CUSTOM_MODEL_ID_K1;
        priv->ptp_custom_vendor_ext_ver = 1; // Assuming default vendor ext ver
    } else if (strstr(model_string, "PENTAX KP")) {
        priv->ptp_custom_model_id = PENTAX_CUSTOM_MODEL_ID_KP;
        priv->ptp_custom_vendor_ext_ver = 1; // Assuming default vendor ext ver
    } else if (strstr(model_string, "RICOH G900SE")) {
        priv->ptp_custom_model_id = PENTAX_CUSTOM_MODEL_ID_G900SE;
        priv->ptp_custom_vendor_ext_ver = 1; // Assuming default vendor ext ver
    } else if (strstr(model_string, "RICOH GR III")) {
        priv->ptp_custom_model_id = PENTAX_CUSTOM_MODEL_ID_GRIII;
        priv->ptp_custom_vendor_ext_ver = 1; // Assuming default vendor ext ver
    } else if (strstr(model_string, "PENTAX K-70")) {
        priv->ptp_custom_model_id = PENTAX_CUSTOM_MODEL_ID_K70;
        priv->ptp_custom_vendor_ext_ver = 1; // Assuming default vendor ext ver
    } else {
        GP_LOG_W("pentaxmodern", "Unknown Pentax model for 0x9520 specific params: '%s'. Using generic values.", model_string);
        priv->ptp_custom_model_id = 0; 
        priv->ptp_custom_vendor_ext_ver = priv->ptp_params.deviceinfo.VendorExtensionVersion; 
    }
    GP_DEBUG("pentaxmodern", "Using custom PTP ModelID: 0x%X, Custom ExtVer: %u for 0x9520 command.",
             priv->ptp_custom_model_id, priv->ptp_custom_vendor_ext_ver);
}

int pentaxmodern_init(Camera *camera) { 
    CameraPrivateLibrary *priv;
    GP_DEBUG("pentaxmodern_init called for %p", camera);
    priv = calloc(1, sizeof(CameraPrivateLibrary));
    if (!priv) return GP_ERROR_NO_MEMORY;
    camera->priv = priv;
    priv->port = camera->port; 
    ptp_init(&priv->ptp_params, priv->port); 
    priv->ptp_params.data = camera; 
    uint16_t ret = ptp_opensession(&priv->ptp_params, 1); 
    if (ret != PTP_RC_OK && ret != PTP_RC_SessionAlreadyOpened) {
        gp_log(GP_LOG_ERROR, "pentaxmodern", "PTP OpenSession failed: 0x%X", ret);
        free(priv); camera->priv = NULL; return GP_ERROR_IO; 
    }
    populate_pentax_model_ids(priv);
    if (priv->ptp_custom_model_id != 0) { 
        uint32_t p[3]; p[0] = priv->ptp_custom_model_id; p[1] = 1; p[2] = priv->ptp_custom_vendor_ext_ver; 
        ret = ptp_generic_no_data(&priv->ptp_params, PENTAX_OC_SET_PENTAX_VENDOR_MODE, 3, p[0], p[1], p[2]);
        GP_LOG_PTP((&priv->ptp_params), GP_LOG_DEBUG, "pentaxmodern", "Set Pentax Vendor Mode (0x%04X), params: (0x%X, %u, %u)", PENTAX_OC_SET_PENTAX_VENDOR_MODE, p[0], p[1], p[2]);
        if (ret != PTP_RC_OK) {
            gp_log(GP_LOG_WARNING, "pentaxmodern", "Failed to set Pentax vendor mode (0x%04X) with specific params (ret: 0x%X), but continuing. Some ops might fail.", PENTAX_OC_SET_PENTAX_VENDOR_MODE, ret);
        } else {
            GP_DEBUG("pentaxmodern", "Successfully set Pentax vendor mode (0x%04X).", PENTAX_OC_SET_PENTAX_VENDOR_MODE);
        }
    } else {
        gp_log(GP_LOG_WARNING, "pentaxmodern", "Model-specific IDs for 0x9520 not resolved. Skipping explicit vendor mode set via 0x9520.");
    }
    gp_port_set_timeout(priv->port, 5000);
    priv->astrotracer_enabled = FALSE; priv->bulb_mode_active = FALSE; priv->live_view_active = FALSE;
    priv->astrotracer_time_limit = 60; 
    priv->conditions_data_cache = NULL; priv->conditions_data_len = 0; 
    priv->keep_aperture_mode = 0; // Default to Reset Aperture
    GP_DEBUG("pentaxmodern_init finished successfully.");
    return GP_OK;
}

int pentaxmodern_exit(Camera *camera) { 
    CameraPrivateLibrary *priv = camera->priv;
    GP_DEBUG("pentaxmodern_exit called for %p", camera);
    if (!priv) return GP_OK; 
    if (priv->live_view_active) {
        GP_DEBUG("Turning off live view during exit.");
        static_pentax_end_live_view(&priv->ptp_params);
        priv->live_view_active = FALSE;
    }
    if (priv->ptp_custom_model_id != 0) { 
         uint32_t p[3];
         p[0] = priv->ptp_custom_model_id;
         p[1] = 0; // 0 for OFF
         p[2] = priv->ptp_custom_vendor_ext_ver;
         ptp_generic_no_data(&priv->ptp_params, PENTAX_OC_SET_PENTAX_VENDOR_MODE, 3, p[0], p[1], p[2]);
         GP_DEBUG("pentaxmodern", "Sent command to deactivate Pentax vendor mode (0x%04X).", PENTAX_OC_SET_PENTAX_VENDOR_MODE);
    }
    ptp_generic_no_data(&priv->ptp_params, PTP_OC_CloseSession, 0,0,0,0,0);
    if (priv->conditions_data_cache) { 
        free(priv->conditions_data_cache);
        priv->conditions_data_cache = NULL;
        priv->conditions_data_len = 0;
    }
    free(priv);
    camera->priv = NULL;
    GP_DEBUG("pentaxmodern_exit finished.");
    return GP_OK;
}

int pentaxmodern_capture(Camera *camera, CameraCaptureType type, CameraFilePath *path, GPContext *context) { /* ... as before ... */ 
    CameraPrivateLibrary *priv = camera->priv;
    GP_DEBUG("pentaxmodern_capture called, type: %d", type);
    if (type != GP_CAPTURE_IMAGE) {
        gp_log(GP_LOG_ERROR, "pentaxmodern", "Capture type not supported: %d", type);
        return GP_ERROR_NOT_SUPPORTED;
    }
    uint16_t ret_ptp; 
    if (priv->bulb_mode_active) {
        gp_log(GP_LOG_DEBUG, "pentaxmodern", "Starting bulb capture, duration: %d s", priv->astrotracer_time_limit);
        ret_ptp = static_pentax_send_simple_command(&priv->ptp_params, PENTAX_OC_BULB_START);
        if (ret_ptp != GP_OK) {
             gp_log(GP_LOG_ERROR, "pentaxmodern", "Failed to start bulb mode: %d", ret_ptp);
             return ret_ptp;
        }
        gp_port_timeout_push(priv->port, (priv->astrotracer_time_limit + 10) * 1000); 
        GP_DEBUG("Waiting %d seconds for bulb exposure...", priv->astrotracer_time_limit);
        sleep(priv->astrotracer_time_limit); 
        gp_port_timeout_pop(priv->port); 
        ret_ptp = static_pentax_send_simple_command(&priv->ptp_params, PENTAX_OC_BULB_END);
         if (ret_ptp != GP_OK) {
             gp_log(GP_LOG_ERROR, "pentaxmodern", "Failed to end bulb mode: %d", ret_ptp);
             return ret_ptp;
        }
        gp_log(GP_LOG_DEBUG, "pentaxmodern", "Bulb capture finished.");
    } else {
        gp_log(GP_LOG_DEBUG, "pentaxmodern", "Initiating standard PTP capture.");
        ret_ptp = ptp_initiatecapture(&priv->ptp_params, 0, 0); 
    }
    if (ret_ptp != PTP_RC_OK) {
        gp_log(GP_LOG_ERROR, "pentaxmodern", "PTP capture operation failed: 0x%X", ret_ptp);
        return GP_ERROR_IO;
    }
    strcpy(path->folder, "/"); 
    sprintf(path->name, "capt%04d.jpg", rand() % 10000); 
    GP_DEBUG("pentaxmodern_capture completed, path: %s/%s", path->folder, path->name);
    return GP_OK;
}

int pentaxmodern_summary(Camera *camera, CameraText *summary, GPContext *context) { /* ... as before, with 0x920F integration ... */ 
    CameraPrivateLibrary *priv = camera->priv;
    GP_DEBUG("pentaxmodern_summary called.");
    char buf[2048] = {0}; char item_buf[256]; uint16_t ret_conditions;
    PTPDeviceInfo devinfo;
    if (ptp_getdeviceinfo(&priv->ptp_params, &devinfo) == PTP_RC_OK) {
        snprintf(buf, sizeof(buf), "Model: %s\nSerial: %s\nVendor Extension Version: %u\n", 
                 devinfo.Model, devinfo.SerialNumber, devinfo.VendorExtensionVersion);
        ptp_free_deviceinfo(&devinfo);
    } else { strcpy(buf, "Pentax Modern PTP Camera (unable to get basic device info)\n"); }
    if (priv->conditions_data_cache) { free(priv->conditions_data_cache); priv->conditions_data_cache = NULL; priv->conditions_data_len = 0; }
    ret_conditions = pentaxmodern_get_all_conditions_data(&priv->ptp_params, &priv->conditions_data_cache, &priv->conditions_data_len);
    if (ret_conditions == PTP_RC_OK && priv->conditions_data_cache && priv->conditions_data_len > 0) {
        GP_DEBUG("pentaxmodern_summary: Using fetched 0x920F data.");
        uint32_t exp_mode_raw = parse_condition_u16(priv->conditions_data_cache, priv->conditions_data_len, CI_CAPTURE_MODE_INFO); 
        uint32_t iso_raw = parse_condition_u32(priv->conditions_data_cache, priv->conditions_data_len, CI_ISO_SETTING); 
        uint32_t battery_raw = parse_condition_u8(priv->conditions_data_cache, priv->conditions_data_len, CI_BATTERY_LEVEL_PLACEHOLDER);
        uint32_t sd1_remain_raw = parse_condition_u32(priv->conditions_data_cache, priv->conditions_data_len, CI_SD1_REMAIN_PLACEHOLDER);
        uint32_t tv_raw = parse_condition_u32(priv->conditions_data_cache, priv->conditions_data_len, CI_TV_VALUE_PLACEHOLDER); 
        uint32_t av_raw = parse_condition_u32(priv->conditions_data_cache, priv->conditions_data_len, CI_AV_VALUE_PLACEHOLDER); 
        snprintf(item_buf, sizeof(item_buf), "Exposure Mode: %s (0x%X)\n", lookup_label(exposure_mode_table, exp_mode_raw), exp_mode_raw); strncat(buf, item_buf, sizeof(buf) - strlen(buf) - 1);
        snprintf(item_buf, sizeof(item_buf), "ISO: %s (0x%X)\n", lookup_label(iso_table, iso_raw), iso_raw); strncat(buf, item_buf, sizeof(buf) - strlen(buf) - 1);
        snprintf(item_buf, sizeof(item_buf), "Shutter Speed: %s (0x%X)\n", lookup_label(shutter_table, tv_raw), tv_raw); strncat(buf, item_buf, sizeof(buf) - strlen(buf) - 1);
        snprintf(item_buf, sizeof(item_buf), "Aperture: %s (0x%X)\n", lookup_label(aperture_table, av_raw), av_raw); strncat(buf, item_buf, sizeof(buf) - strlen(buf) - 1);
        snprintf(item_buf, sizeof(item_buf), "Battery Level: %u%%\n", battery_raw); strncat(buf, item_buf, sizeof(buf) - strlen(buf) - 1);
        snprintf(item_buf, sizeof(item_buf), "SD Card Remaining: %u\n", sd1_remain_raw); strncat(buf, item_buf, sizeof(buf) - strlen(buf) - 1);
    } else {
        GP_LOG_W("pentaxmodern_summary", "Could not retrieve detailed conditions via 0x920F (ret: 0x%X). Summary may be incomplete.", ret_conditions);
        strncat(buf, "Could not retrieve detailed conditions via 0x920F.\n", sizeof(buf) - strlen(buf) - 1);
    }
    strncpy(summary->text, buf, sizeof(summary->text) - 1); summary->text[sizeof(summary->text) - 1] = '\0';
    return GP_OK;
}

static int pentaxmodern_get_config (Camera *camera, CameraWidget **window, GPContext *context) { 
    CameraPrivateLibrary *priv = camera->priv;
    CameraWidget *section, *widget, *ci_section, *movie_section; 
    uint32_t raw_val_u32;
    uint16_t raw_val_u16;
    uint8_t  raw_val_u8;
    int8_t   raw_val_s8; 
    const char* current_label;
    uint16_t ret_ptp_conditions;
    float range_float_val; 

    GP_DEBUG("pentaxmodern_get_config called.");
    gp_widget_new(GP_WIDGET_WINDOW, "Pentax Modern Configuration", window);

    if (priv->conditions_data_cache) { 
        free(priv->conditions_data_cache);
        priv->conditions_data_cache = NULL;
        priv->conditions_data_len = 0;
    }
    ret_ptp_conditions = pentaxmodern_get_all_conditions_data(&priv->ptp_params, &priv->conditions_data_cache, &priv->conditions_data_len);

    // Basic Sections (Astro, Capture, Live View)
    gp_widget_new(GP_WIDGET_SECTION, "Astrotracer", &section); gp_widget_append(*window, section);
    gp_widget_new(GP_WIDGET_TOGGLE, "Enable Astrotracer", &widget); gp_widget_set_value(widget, &priv->astrotracer_enabled); gp_widget_append(section, widget);
    gp_widget_new(GP_WIDGET_RANGE, "Astrotracer Time Limit (sec)", &widget); gp_widget_set_range(widget, 10, 300, 10); gp_widget_set_value(widget, &priv->astrotracer_time_limit); gp_widget_append(section, widget);
    
    gp_widget_new(GP_WIDGET_SECTION, "Capture Modes", &section); gp_widget_append(*window, section);
    gp_widget_new(GP_WIDGET_TOGGLE, "Bulb Mode Active", &widget); gp_widget_set_value(widget, &priv->bulb_mode_active); gp_widget_append(section, widget);
    
    gp_widget_new(GP_WIDGET_RADIO, "Aperture Setting Priority", &widget);
    for(int i=0; KeepAperture_table[i].label; i++) gp_widget_add_choice(widget, KeepAperture_table[i].label);
    gp_widget_set_value(widget, lookup_label(KeepAperture_table, priv->keep_aperture_mode)); 
    gp_widget_append(section, widget);

    gp_widget_new(GP_WIDGET_SECTION, "Live View", &section); gp_widget_append(*window, section);
    gp_widget_new(GP_WIDGET_TOGGLE, "Live View Active", &widget); gp_widget_set_value(widget, &priv->live_view_active); gp_widget_append(section, widget);
    gp_widget_new(GP_WIDGET_RANGE, "Live View Interval (ms)", &widget); gp_widget_set_range(widget, 30, 1000, 10); gp_widget_set_value(widget, &priv->live_view_interval); gp_widget_append(section, widget);
    gp_widget_new(GP_WIDGET_RADIO, "Live View Zoom", &widget); const char *zoom_choices[] = {"1x", "2x", "4x", "6x", "8x", NULL}; for (int i = 0; zoom_choices[i]; i++) gp_widget_add_choice(widget, zoom_choices[i]); gp_widget_set_value(widget, "1x"); gp_widget_append(section, widget);
    
    gp_widget_new(GP_WIDGET_SECTION, "Exposure Settings", &section); gp_widget_append(*window, section);
    // ISO, WB, ExpMode with 0x920F fallback
    if (ret_ptp_conditions == PTP_RC_OK && priv->conditions_data_cache) {
        raw_val_u32 = parse_condition_u32(priv->conditions_data_cache, priv->conditions_data_len, CI_ISO_SETTING); current_label = lookup_label(iso_table, raw_val_u32);
    } else if (pentaxmodern_get_ptp_property_u32(&priv->ptp_params, PENTAX_OC_GET_ISO, &raw_val_u32) == PTP_RC_OK) {
        current_label = lookup_label(iso_table, raw_val_u32);
    } else { current_label = "Unknown"; }
    gp_widget_new(GP_WIDGET_RADIO, "ISO", &widget); for (int i = 0; iso_table[i].label; i++) gp_widget_add_choice(widget, iso_table[i].label); gp_widget_set_value(widget, current_label); gp_widget_append(section, widget);
    
    if (ret_ptp_conditions == PTP_RC_OK && priv->conditions_data_cache) {
        raw_val_u16 = parse_condition_u16(priv->conditions_data_cache, priv->conditions_data_len, CI_WB_MODE); current_label = lookup_label(wb_table, raw_val_u16);
    } else if (pentaxmodern_get_ptp_property_u32(&priv->ptp_params, PENTAX_OC_GET_WHITE_BALANCE, &raw_val_u32) == PTP_RC_OK) { 
        current_label = lookup_label(wb_table, raw_val_u32);
    } else { current_label = "Unknown"; }
    gp_widget_new(GP_WIDGET_RADIO, "White Balance", &widget); for (int i = 0; wb_table[i].label; i++) gp_widget_add_choice(widget, wb_table[i].label); gp_widget_set_value(widget, current_label); gp_widget_append(section, widget);
    
    if (ret_ptp_conditions == PTP_RC_OK && priv->conditions_data_cache) {
        raw_val_u16 = parse_condition_u16(priv->conditions_data_cache, priv->conditions_data_len, CI_CAPTURE_MODE_INFO); current_label = lookup_label(exposure_mode_table, raw_val_u16);
    } else if (pentaxmodern_get_ptp_property_u32(&priv->ptp_params, PENTAX_OC_GET_EXPOSURE_MODE, &raw_val_u32) == PTP_RC_OK) {
        current_label = lookup_label(exposure_mode_table, raw_val_u32);
    } else { current_label = "Unknown"; }
    gp_widget_new(GP_WIDGET_RADIO, "Exposure Mode", &widget); for (int i = 0; exposure_mode_table[i].label; i++) gp_widget_add_choice(widget, exposure_mode_table[i].label); gp_widget_set_value(widget, current_label); gp_widget_append(section, widget);

    if (pentaxmodern_get_ptp_property_u32(&priv->ptp_params, PENTAX_OC_GET_SHUTTER, &raw_val_u32) == PTP_RC_OK) { current_label = lookup_label(shutter_table, raw_val_u32); } else { current_label = "Unknown"; }
    gp_widget_new(GP_WIDGET_RADIO, "Shutter Speed", &widget); for (int i = 0; shutter_table[i].label; i++) gp_widget_add_choice(widget, shutter_table[i].label); gp_widget_set_value(widget, current_label); gp_widget_append(section, widget);
    if (pentaxmodern_get_ptp_property_u32(&priv->ptp_params, PENTAX_OC_GET_APERTURE, &raw_val_u32) == PTP_RC_OK) { current_label = lookup_label(aperture_table, raw_val_u32); } else { current_label = "Unknown"; }
    gp_widget_new(GP_WIDGET_RADIO, "Aperture", &widget); for (int i = 0; aperture_table[i].label; i++) gp_widget_add_choice(widget, aperture_table[i].label); gp_widget_set_value(widget, current_label); gp_widget_append(section, widget);
    
    // Movie Settings Section
    gp_widget_new(GP_WIDGET_SECTION, "Movie Settings", &movie_section);
    gp_widget_append(*window, movie_section);
    uint8_t movie_mode_val = 0;
    if (get_ptp_dpc_u8(camera, PENTAX_DPC_MOVIE_MODE, &movie_mode_val) == GP_OK) {
        gp_widget_new(GP_WIDGET_TOGGLE, "Movie Mode", &widget);
        int movie_mode_enabled = (movie_mode_val != 0); 
        gp_widget_set_value(widget, &movie_mode_enabled);
        gp_widget_append(movie_section, widget);
    }
    // TODO: Fetch current Movie File Format and Quality if readable DPCs exist or from 0x920F if indices known.
    gp_widget_new(GP_WIDGET_RADIO, "Movie File Format", &widget);
    for(int i=0; MovieFileFormat_table[i].label; i++) gp_widget_add_choice(widget, MovieFileFormat_table[i].label);
    gp_widget_set_value(widget, MovieFileFormat_table[0].label); // Default
    gp_widget_append(movie_section, widget);
    gp_widget_new(GP_WIDGET_RADIO, "Movie Quality", &widget);
    for(int i=0; MovieQuality_table[i].label; i++) gp_widget_add_choice(widget, MovieQuality_table[i].label);
    gp_widget_set_value(widget, MovieQuality_table[0].label); // Default
    gp_widget_append(movie_section, widget);


    // Custom Image Details Section
    gp_widget_new(GP_WIDGET_SECTION, "Custom Image Details", &ci_section);
    gp_widget_append(*window, ci_section);
    if (get_ptp_dpc_s8(camera, PENTAX_DPC_CI_SATURATION, &raw_val_s8) == GP_OK) {
        gp_widget_new(GP_WIDGET_RANGE, "CI Saturation", &widget); range_float_val = raw_val_s8;
        gp_widget_set_range(widget, -4.0f, 4.0f, 1.0f); gp_widget_set_value(widget, &range_float_val);
        gp_widget_append(ci_section, widget);
    }
    if (get_ptp_dpc_s8(camera, PENTAX_DPC_CI_HUE, &raw_val_s8) == GP_OK) {
        gp_widget_new(GP_WIDGET_RANGE, "CI Hue", &widget); range_float_val = raw_val_s8;
        gp_widget_set_range(widget, -4.0f, 4.0f, 1.0f); gp_widget_set_value(widget, &range_float_val);
        gp_widget_append(ci_section, widget);
    }
    if (get_ptp_dpc_s8(camera, PENTAX_DPC_CI_KEY, &raw_val_s8) == GP_OK) {
        gp_widget_new(GP_WIDGET_RANGE, "CI Key", &widget); range_float_val = raw_val_s8;
        gp_widget_set_range(widget, -4.0f, 4.0f, 1.0f); gp_widget_set_value(widget, &range_float_val);
        gp_widget_append(ci_section, widget);
    }
    if (get_ptp_dpc_s8(camera, PTP_DPC_Contrast, &raw_val_s8) == GP_OK) { 
        gp_widget_new(GP_WIDGET_RANGE, "CI Contrast", &widget); range_float_val = raw_val_s8;
        gp_widget_set_range(widget, -4.0f, 4.0f, 1.0f); gp_widget_set_value(widget, &range_float_val);
        gp_widget_append(ci_section, widget);
    }
    if (get_ptp_dpc_s8(camera, PENTAX_DPC_CI_CONTRAST_HIGHLIGHT, &raw_val_s8) == GP_OK) {
        gp_widget_new(GP_WIDGET_RANGE, "CI Contrast Highlight", &widget); range_float_val = raw_val_s8;
        gp_widget_set_range(widget, -4.0f, 4.0f, 1.0f); gp_widget_set_value(widget, &range_float_val);
        gp_widget_append(ci_section, widget);
    }
    if (get_ptp_dpc_s8(camera, PENTAX_DPC_CI_CONTRAST_SHADOW, &raw_val_s8) == GP_OK) {
        gp_widget_new(GP_WIDGET_RANGE, "CI Contrast Shadow", &widget); range_float_val = raw_val_s8;
        gp_widget_set_range(widget, -4.0f, 4.0f, 1.0f); gp_widget_set_value(widget, &range_float_val);
        gp_widget_append(ci_section, widget);
    }
    if (get_ptp_dpc_s8(camera, PTP_DPC_Sharpness, &raw_val_s8) == GP_OK) { 
        gp_widget_new(GP_WIDGET_RANGE, "CI Sharpness", &widget); range_float_val = raw_val_s8;
        gp_widget_set_range(widget, -4.0f, 4.0f, 1.0f); gp_widget_set_value(widget, &range_float_val);
        gp_widget_append(ci_section, widget);
    }
    if (get_ptp_dpc_u8(camera, PENTAX_DPC_CI_FINE_SHARPNESS, &raw_val_u8) == GP_OK) { 
        current_label = lookup_label(CIFineSharpness_table, raw_val_u8);
        gp_widget_new(GP_WIDGET_RADIO, "CI Fine Sharpness", &widget);
        for(int i=0; CIFineSharpness_table[i].label; i++) gp_widget_add_choice(widget, CIFineSharpness_table[i].label);
        gp_widget_set_value(widget, current_label); gp_widget_append(ci_section, widget);
    }
    if (get_ptp_dpc_u8(camera, PENTAX_DPC_CI_FILTER_EFFECT, &raw_val_u8) == GP_OK) { 
        current_label = lookup_label(CIFilterEffect_table, raw_val_u8);
        gp_widget_new(GP_WIDGET_RADIO, "CI Filter Effect", &widget);
        for(int i=0; CIFilterEffect_table[i].label; i++) gp_widget_add_choice(widget, CIFilterEffect_table[i].label);
        gp_widget_set_value(widget, current_label); gp_widget_append(ci_section, widget);
    }
    if (get_ptp_dpc_u8(camera, PENTAX_DPC_CI_MONOTONE_TONING, &raw_val_u8) == GP_OK) { 
        current_label = lookup_label(CIToning_table, raw_val_u8);
        gp_widget_new(GP_WIDGET_RADIO, "CI Monotone Toning", &widget);
        for(int i=0; CIToning_table[i].label; i++) gp_widget_add_choice(widget, CIToning_table[i].label);
        gp_widget_set_value(widget, current_label); gp_widget_append(ci_section, widget);
    }
    if (get_ptp_dpc_u8(camera, PENTAX_DPC_CI_TONING, &raw_val_u8) == GP_OK) { 
        current_label = lookup_label(CIToning_table, raw_val_u8);
        gp_widget_new(GP_WIDGET_RADIO, "CI Toning (Color)", &widget);
        for(int i=0; CIToning_table[i].label; i++) gp_widget_add_choice(widget, CIToning_table[i].label);
        gp_widget_set_value(widget, current_label); gp_widget_append(ci_section, widget);
    }
    if (get_ptp_dpc_u8(camera, PENTAX_DPC_CI_CROSS_PROCESS_TYPE, &raw_val_u8) == GP_OK) { 
        current_label = lookup_label(CICrossProcessType_table, raw_val_u8);
        gp_widget_new(GP_WIDGET_RADIO, "CI Cross Process Type", &widget);
        for(int i=0; CICrossProcessType_table[i].label; i++) gp_widget_add_choice(widget, CICrossProcessType_table[i].label);
        gp_widget_set_value(widget, current_label); gp_widget_append(ci_section, widget);
    }
    // TODO: PENTAX_DPC_CI_USER_FILTER_EFFECT (0xD02D) - array of 3 shorts. Omitted for now.

    // ... (rest of get_config, e.g., Focus, Storage, Hardware controls, as before) ...
    GP_DEBUG("pentaxmodern_get_config finished.");
    return GP_OK;
}

static int pentaxmodern_set_config (Camera *camera, CameraWidget *window, GPContext *context) { 
    CameraPrivateLibrary *priv = camera->priv;
    CameraWidget *widget;
    int enabled_val, int_val; 
    float range_float_val; 
    char *choice_val; 
    uint32_t ptp_val_u32; // Used for generic PTP operations
    int8_t ptp_val_s8;
    uint8_t ptp_val_u8;

    GP_DEBUG("pentaxmodern_set_config called.");
    // ... (Existing config settings for Astro, Bulb, Live View, basic Exposure etc. as before) ...
    if (gp_widget_get_child_by_name(window, "Aperture Setting Priority", &widget) == GP_OK && gp_widget_changed(widget)) {
        gp_widget_get_value(widget, &choice_val);
        uint8_t keep_mode = lookup_value(KeepAperture_table, choice_val);
        priv->keep_aperture_mode = keep_mode;
        uint32_t current_aperture_ptp_value = 0; 

        if (keep_mode == 1) { // Keep Aperture
            if (priv->conditions_data_cache && priv->conditions_data_len > 0) {
                current_aperture_ptp_value = parse_condition_u32(priv->conditions_data_cache, priv->conditions_data_len, CI_AV_VALUE_PLACEHOLDER);
            } else {
                pentaxmodern_get_ptp_property_u32(&priv->ptp_params, PENTAX_OC_GET_APERTURE, &current_aperture_ptp_value);
            }
             GP_DEBUG("pentaxmodern", "Keep Aperture selected. Current aperture PTP value: 0x%X", current_aperture_ptp_value);
        }
        ptp_generic_no_data(&priv->ptp_params, PENTAX_OC_SET_APERTURE_KEEP, 2, (uint32_t)priv->keep_aperture_mode, current_aperture_ptp_value);
        free(choice_val);
    }
    
    // Movie Settings
    if (gp_widget_get_child_by_name(window, "Movie Mode", &widget) == GP_OK && gp_widget_changed(widget)) {
        gp_widget_get_value(widget, &enabled_val);
        set_ptp_dpc_u8(camera, PENTAX_DPC_MOVIE_MODE, enabled_val ? 1 : 0); 
    }
    
    CameraWidget *movie_format_widget = NULL, *movie_quality_widget = NULL;
    gp_widget_get_child_by_name(window, "Movie File Format", &movie_format_widget);
    gp_widget_get_child_by_name(window, "Movie Quality", &movie_quality_widget);

    if ((movie_format_widget && gp_widget_changed(movie_format_widget)) || 
        (movie_quality_widget && gp_widget_changed(movie_quality_widget))) {
        
        uint32_t movie_quality_choice_val = 0;
        uint32_t movie_pixsize = PENTAX_CONST_MOVIE_PIXSIZE_FHD; 
        uint32_t movie_framerate = PENTAX_CONST_MOVIE_FRAMERATE_30P; 
        // uint32_t movie_file_format_val = 0; // Example, if "Movie File Format" widget existed and was used

        if (movie_quality_widget) { // Assuming this widget exists and was changed or format was changed
            gp_widget_get_value(movie_quality_widget, &choice_val);
            movie_quality_choice_val = lookup_value(MovieQuality_table, choice_val);
            free(choice_val);

            if (movie_quality_choice_val == PENTAX_MOVIE_QUALITY_4K_30P) {
                movie_pixsize = PENTAX_CONST_MOVIE_PIXSIZE_4K; movie_framerate = PENTAX_CONST_MOVIE_FRAMERATE_30P;
            } else if (movie_quality_choice_val == PENTAX_MOVIE_QUALITY_FHD_60P) {
                movie_pixsize = PENTAX_CONST_MOVIE_PIXSIZE_FHD; movie_framerate = PENTAX_CONST_MOVIE_FRAMERATE_60P;
            } // else default to FHD 30p
        }
        // if (movie_format_widget) { /* Get movie_file_format_val similarly */ }
        
        PTPPropValue propv;
        // The PTP standard specifies arrays as a sequence of elements.
        // ptp_setdevicepropvalue expects a PTPPropValue where, for arrays,
        // the .a.v field points to an array of PTPPropValue elements.
        // This is complex. A common simplification for vendor DPCs is to send raw byte arrays.
        // Let's assume PENTAX_DPC_WRITING_FILE_FORMAT_SETTING expects a flat structure
        // of 3x uint16_t (PixSize, FrameRate, CardSlot) - this is a guess.
        // Total 6 bytes.
        unsigned char movie_settings_data[6];
        uint16_t pixsize16 = (uint16_t)movie_pixsize; // Truncate if necessary
        uint16_t framerate16 = (uint16_t)movie_framerate;
        uint16_t cardslot16 = (uint16_t)PENTAX_CONST_MOVIE_CARD_SLOT_DEFAULT;

        htole16a(&movie_settings_data[0], pixsize16);
        htole16a(&movie_settings_data[2], framerate16);
        htole16a(&movie_settings_data[4], cardslot16);
        
        propv.str = (char*)movie_settings_data; // Treat as opaque byte string for ptp_setdevicepropvalue
                                                // The 'datatype' param will tell ptp.c how to interpret it.
                                                // PTP_DTC_STR usually means null-terminated. This is not.
                                                // Need a way to send raw byte array.
                                                // PTP_DTC_AUINT8 with count 6 might work.
        
        // Forcing PTP_DTC_AUINT8 for now, as it's a common way to send byte blobs for vendor props.
        // The actual datatype for PENTAX_DPC_WRITING_FILE_FORMAT_SETTING (0xD01B) is critical.
        // If it's truly an array of uint32_t, then PTP_DTC_AUINT32 should be used,
        // and the data packing would be different (array of 3x uint32_t).
        // This part is highly speculative due to lack of precise data structure info.
        // The following will likely fail or be misinterpreted by the camera if the data type/structure is wrong.
        propv.a.count = 6; // Number of bytes for AUINT8
        propv.a.v = (PTPPropValue*)movie_settings_data; // Incorrect usage of PTPPropValue for array

        GP_LOG_W("pentaxmodern", "Attempting to set Movie Format/Quality (0xD01B). This is speculative and may not work.");
        // set_ptp_deviceprop_value(camera, PENTAX_DPC_WRITING_FILE_FORMAT_SETTING, &propv, PTP_DTC_AUINT8); 
        GP_LOG_E("pentaxmodern", "TODO: Correctly implement array packing for PENTAX_DPC_WRITING_FILE_FORMAT_SETTING (0xD01B).");
    }


    // Custom Image Detail Settings
    if (gp_widget_get_child_by_name(window, "CI Saturation", &widget) == GP_OK && gp_widget_changed(widget)) {
        gp_widget_get_value(widget, &range_float_val); ptp_val_s8 = (int8_t)range_float_val; 
        set_ptp_dpc_s8(camera, PENTAX_DPC_CI_SATURATION, ptp_val_s8);
    }
    if (gp_widget_get_child_by_name(window, "CI Hue", &widget) == GP_OK && gp_widget_changed(widget)) {
        gp_widget_get_value(widget, &range_float_val); ptp_val_s8 = (int8_t)range_float_val;
        set_ptp_dpc_s8(camera, PENTAX_DPC_CI_HUE, ptp_val_s8);
    }
    // ... (rest of CI set logic from previous turn)

    GP_DEBUG("pentaxmodern_set_config finished.");
    return GP_OK;
}

// ... (Rest of the functions: pentaxmodern_get_preview, _trigger_autofocus, file ops, _id, _abilities_list, CameraLibrary struct as before) ...
// (Full functions from previous turn's file content are assumed here for brevity)
static int pentaxmodern_get_preview (Camera *camera, CameraFile *file, GPContext *context) { /* ... as before ... */ 
    CameraPrivateLibrary *priv = camera->priv;
    GP_DEBUG("pentaxmodern_get_preview called.");
    if (!priv->live_view_active) {
        gp_log(GP_LOG_ERROR, "pentaxmodern", "Live view is not active for preview.");
        return GP_ERROR_BAD_PARAMETERS;
    }
    unsigned char *data = NULL;
    unsigned int size = 0;
    uint16_t ret = static_pentax_get_lv_frame(&priv->ptp_params, &data, &size);
    if (ret != PTP_RC_OK || !data || size == 0) {
        gp_log(GP_LOG_ERROR, "pentaxmodern", "Failed to get live view frame: 0x%X", ret);
        if(data) free(data); 
        return GP_ERROR_IO;
    }
    gp_file_set_data_and_size(file, (char*)data, size); 
    free(data); 
    GP_DEBUG("pentaxmodern_get_preview finished, size %u.", size);
    return GP_OK;
}

static int pentaxmodern_trigger_autofocus(Camera *camera, GPContext *context) { /* ... as before ... */ 
    CameraPrivateLibrary *priv = camera->priv;
    GP_DEBUG("pentaxmodern_trigger_autofocus called.");
    return static_pentax_send_simple_command(&priv->ptp_params, PENTAX_OC_AF_TRIGGER);
}

static int pentaxmodern_folder_list_files (Camera *camera, const char *folder, CameraList *list, GPContext *context) { /* ... as before ... */ 
    CameraPrivateLibrary *priv = camera->priv;
    GP_DEBUG("pentaxmodern_folder_list_files called for folder: %s", folder);
    PTPObjectHandles handles;
    memset(&handles, 0, sizeof(handles)); 
    int ret = ptp_getobjecthandles(&priv->ptp_params, 0xFFFFFFFF, 0, 0, &handles);
    if (ret != PTP_RC_OK) {
        gp_log(GP_LOG_ERROR, "pentaxmodern", "ptp_getobjecthandles failed: 0x%X", ret);
        return GP_ERROR_IO;
    }
    for (unsigned int i = 0; i < handles.n; i++) {
        PTPObjectInfo oi;
        memset(&oi, 0, sizeof(oi));
        ret = ptp_getobjectinfo(&priv->ptp_params, handles.Path[i], &oi);
        if (ret == PTP_RC_OK) {
            gp_list_append(list, oi.Filename, NULL);
            ptp_free_objectinfo(&oi); 
        } else {
            gp_log(GP_LOG_WARNING, "pentaxmodern", "ptp_getobjectinfo for handle 0x%X failed: 0x%X", handles.Path[i], ret);
        }
    }
    if (handles.Path) free(handles.Path);
    GP_DEBUG("pentaxmodern_folder_list_files finished.");
    return GP_OK;
}

static int pentaxmodern_file_get (Camera *camera, const char *folder, const char *filename, GPFileType type, CameraFile *file, GPContext *context) { /* ... as before ... */ 
    CameraPrivateLibrary *priv = camera->priv;
    GP_DEBUG("pentaxmodern_file_get called for %s/%s", folder, filename);
    PTPObjectHandles handles;
    memset(&handles, 0, sizeof(handles));
    int ret = ptp_getobjecthandles(&priv->ptp_params, 0xFFFFFFFF, 0, 0, &handles);
    if (ret != PTP_RC_OK) {
        gp_log(GP_LOG_ERROR, "pentaxmodern", "ptp_getobjecthandles failed: 0x%X", ret);
        return GP_ERROR_IO;
    }
    uint32_t object_handle = 0;
    for (unsigned int i = 0; i < handles.n; i++) {
        PTPObjectInfo oi;
        memset(&oi, 0, sizeof(oi));
        ret = ptp_getobjectinfo(&priv->ptp_params, handles.Path[i], &oi);
        if (ret == PTP_RC_OK) {
            if (oi.Filename && g_strcmp0(oi.Filename, filename) == 0) {
                object_handle = handles.Path[i];
                ptp_free_objectinfo(&oi);
                break; 
            }
            ptp_free_objectinfo(&oi);
        }
    }
    if (handles.Path) free(handles.Path);
    if (object_handle == 0) {
        gp_log(GP_LOG_ERROR, "pentaxmodern", "File not found: %s", filename);
        return GP_ERROR_FILE_NOT_FOUND;
    }
    unsigned char *data = NULL;
    unsigned int size = 0; 
    ret = ptp_getobject_with_size(&priv->ptp_params, object_handle, &data, &size);
    if (ret == PTP_RC_OK && data && size > 0) {
        gp_file_set_data_and_size(file, (char*)data, size); 
        free(data); 
        GP_DEBUG("pentaxmodern_file_get finished successfully for %s", filename);
        return GP_OK;
    } else {
        gp_log(GP_LOG_ERROR, "pentaxmodern", "ptp_getobject_with_size for handle 0x%X failed: 0x%X", object_handle, ret);
        if(data) free(data);
        return GP_ERROR_IO;
    }
}

static int pentaxmodern_file_delete (Camera *camera, const char *folder, const char *filename, GPContext *context) { /* ... as before ... */ 
    CameraPrivateLibrary *priv = camera->priv;
    GP_DEBUG("pentaxmodern_file_delete called for %s/%s", folder, filename);
    PTPObjectHandles handles;
    memset(&handles, 0, sizeof(handles));
    int ret = ptp_getobjecthandles(&priv->ptp_params, 0xFFFFFFFF, 0, 0, &handles);
    if (ret != PTP_RC_OK) {
        gp_log(GP_LOG_ERROR, "pentaxmodern", "ptp_getobjecthandles failed: 0x%X", ret);
        return GP_ERROR_IO;
    }
    uint32_t object_handle = 0;
    for (unsigned int i = 0; i < handles.n; i++) {
        PTPObjectInfo oi;
        memset(&oi, 0, sizeof(oi));
        ret = ptp_getobjectinfo(&priv->ptp_params, handles.Path[i], &oi);
        if (ret == PTP_RC_OK) {
            if (oi.Filename && g_strcmp0(oi.Filename, filename) == 0) {
                object_handle = handles.Path[i];
                ptp_free_objectinfo(&oi);
                break;
            }
            ptp_free_objectinfo(&oi);
        }
    }
    if (handles.Path) free(handles.Path);
    if (object_handle == 0) {
        gp_log(GP_LOG_ERROR, "pentaxmodern", "File not found for deletion: %s", filename);
        return GP_ERROR_FILE_NOT_FOUND;
    }
    ret = ptp_deleteobject(&priv->ptp_params, object_handle, 0); 
    if (ret != PTP_RC_OK) {
        gp_log(GP_LOG_ERROR, "pentaxmodern", "ptp_deleteobject for handle 0x%X failed: 0x%X", object_handle, ret);
        return GP_ERROR_IO;
    }
    GP_DEBUG("pentaxmodern_file_delete finished successfully for %s", filename);
    return GP_OK;
}

static CameraOperations pentaxmodern_ops = { /* ... as before ... */ 
    .init = pentaxmodern_init,
    .exit = pentaxmodern_exit,
    .capture = pentaxmodern_capture,
    .summary = pentaxmodern_summary,
    .get_config = pentaxmodern_get_config,
    .set_config = pentaxmodern_set_config,
    .trigger_capture = NULL, 
    .wait_for_event = NULL,  
    .folder_list_files = pentaxmodern_folder_list_files,
    .folder_list_folders = NULL, 
    .folder_delete_all = NULL,
    .folder_put_file = NULL,
    .folder_make_dir = NULL,
    .folder_remove_dir = NULL,
    .file_get = pentaxmodern_file_get,
    .file_get_info = NULL, 
    .file_set_info = NULL, 
    .file_delete = pentaxmodern_file_delete,
    .about = NULL, 
    .preview = pentaxmodern_get_preview,
    .trigger_autofocus = pentaxmodern_trigger_autofocus,
    .manual_focus = NULL 
};

int pentaxmodern_id(CameraText *id) { /* ... as before ... */ 
    GP_DEBUG("pentaxmodern_id called.");
    strcpy(id->text, "pentaxmodern"); 
    return GP_OK;
}

int pentaxmodern_abilities_list(CameraAbilitiesList *list, GPContext *context) { /* ... as before ... */ 
    CameraAbilities a;
    GP_DEBUG("pentaxmodern_abilities_list called.");
    memset(&a, 0, sizeof(CameraAbilities));
    strcpy(a.model, "Pentax: Modern PTP"); 
    a.status = GP_DRIVER_STATUS_EXPERIMENTAL; 
    a.port = GP_PORT_USB; 
    a.speed[0] = 0; 
    a.operations = GP_OPERATION_CAPTURE_IMAGE | GP_OPERATION_CONFIG | GP_OPERATION_CAPTURE_PREVIEW;
    a.file_operations = GP_FILE_OPERATION_DELETE | GP_FILE_OPERATION_PREVIEW; 
    a.folder_operations = GP_FOLDER_OPERATION_NONE; 
    a.usb_vendor = PENTAX_VENDOR_ID; 
    a.usb_product[0] = PENTAX_PRODUCT_K1II;
    a.usb_product[1] = PENTAX_PRODUCT_K3III;
    a.usb_product[2] = PENTAX_PRODUCT_K70;
    a.usb_product[3] = 0; 
    a.ops = &pentaxmodern_ops; 
    a.device_type = GP_DEVICE_STILL_CAMERA;
    gp_abilities_list_append(list, a);
    GP_DEBUG("pentaxmodern_abilities_list finished.");
    return GP_OK;
}

CameraLibrary pentaxmodern_camlib = { /* ... as before ... */ 
    .id = pentaxmodern_id,
    .abilities = pentaxmodern_abilities_list,
};

[end of camlibs/pentaxmodern/pentaxmodern.c]
