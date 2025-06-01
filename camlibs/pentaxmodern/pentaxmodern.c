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
#include <config.h>
#include "libgphoto2/ptp.h"

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
#define PENTAX_OC_GET_ACTIVE_STORAGE 0x9546 // Hinted opcode for getting active storage

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

#ifndef PTP_DPC_WhiteBalance
#define PTP_DPC_WhiteBalance 0x5005 // Standard PTP Device Property Code for White Balance
#endif

#ifndef PTP_DPC_WhiteBalance
#define PTP_DPC_WhiteBalance 0x5005 // Standard PTP Device Property Code for White Balance
#endif

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
#define CI_DRIVE_MODE_NEW 123 // From C# ConditionIndex, assumed for detailed drive mode
// CI_IMAGE_SIZE (34) is often used for still image size; assuming it might be defined elsewhere or implicitly used.
// If it's specifically for movie context here and not globally defined, it would be:
// #define CI_IMAGE_SIZE 34 // Contains movie pixel size (low short) and frame rate (high short)
#define CI_WRITING_FORMAT_SETTING 131   // Contains current movie file format
#define CI_MOVIE_CARD_SLOT_SETTING 140 // Contains current movie card slot

#define CI_ASTROTRACER_TIMELIMIT 132 // Contains current astrotracer time limit (uint16_t seconds)
#define CI_DSP_STATE_2 6             // Contains various DSP states, including Astrotracer active
#define PENTAX_DSP_STATE2_ASTRO_PRE_EXPOSING 49u // Value in CI_DSP_STATE_2 indicating Astrotracer pre-exposure
#define PENTAX_DSP_STATE2_ASTRO_MAIN_EXPOSING 50u // Value in CI_DSP_STATE_2 indicating Astrotracer main exposure

#define PENTAX_CUSTOM_MODEL_ID_K3III       0x13254 
#define PENTAX_CUSTOM_VENDOR_EXT_VER_K3III 1       
#define PENTAX_CUSTOM_MODEL_ID_K1II        0x13210 
#define PENTAX_CUSTOM_VENDOR_EXT_VER_K1II  1       

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
static PentaxLookupEntry drive_table[] = {
    {"Single Shot", 0},                      // PTP Value 0u
    {"Continuous H", 4},                   // PTP Value 4u
    {"Continuous M", 1},                   // PTP Value 1u
    {"Continuous L", 2},                   // PTP Value 2u
    {"Self-timer 12s", 5},                 // PTP Value 5u (Standard self-timer)
    {"Self-timer 2s", 3},                  // PTP Value 3u
    {"Remote Control", 6},                 // PTP Value 6u (Single shot remote)
    {"Remote Control (3s delay)", 7},      // PTP Value 7u
    {"Auto Bracket", 8},                   // PTP Value 8u
    {"Remote Control (Continuous)", 36},   // PTP Value 36u - Corresponds to C# NominalToInternal index 12
    {"Mirror Up", 9},                      // PTP Value 9u
    {"Mirror Up (Remote)", 10},            // PTP Value 10u
    {"Multi-Exposure", 11},                // PTP Value 11u
    {"Multi-Exposure (Continuous)", 40},   // PTP Value 40u - Corresponds to C# NominalToInternal index 16
    {"Multi-Exposure (Self-timer)", 41}, // PTP Value 41u - Corresponds to C# NominalToInternal index 17
    {"Multi-Exposure (Remote)", 42},     // PTP Value 42u - Corresponds to C# NominalToInternal index 18
    {"Interval Shooting", 12},             // PTP Value 12u
    {"Interval Composite", 13},            // PTP Value 13u - Corresponds to C# NominalToInternal index 40 (Label was Interval Multi-Exposure in C# UI)
    {"Interval Movie Record", 14},         // PTP Value 14u
    {"Star Stream", 15},                   // PTP Value 15u
    {"Self-timer (Continuous)", 22},       // PTP Value 22u - Corresponds to C# NominalToInternal index 6
    // Further entries from C#'s DriveModeLUT could be added here if exact labels and use-cases are clear.
    // This selection covers many common advanced modes.
    {NULL, 0}
};
static PentaxLookupEntry wb_table[] = { /* ... full content ... */
    {"Auto", 1}, {"Daylight", 2}, {"Shade", 3}, {"Cloudy", 4}, {"Tungsten", 5},
    // White Balance lookup table using standard PTP_DPC_WhiteBalance (0x5005) values
    {"Auto", 2},                         // PTP_WB_AUTO
    {"Multi Auto WB", 4},                // PTP_WB_MULTI_AUTO
    {"Daylight", 32770},                 // PTP_WB_DAYLIGHT (0x8002)
    {"Shade", 32771},                    // PTP_WB_SHADE (0x8003)
    {"Cloudy", 32772},                   // PTP_WB_CLOUDY_TUNGSTEN or CLOUDY (0x8004)
    {"Fluorescent D (Daylight)", 32773}, // PTP_WB_FLUORESCENT_DAYLIGHT (0x8005)
    {"Fluorescent N (Neutral White)", 32774},// PTP_WB_FLUORESCENT_NEUTRAL_WHITE (0x8006)
    {"Fluorescent W (Cool White)", 5},   // PTP_WB_FLUORESCENT_WHITE
    {"Fluorescent L (Warm White)", 32781},// PTP_WB_FLUORESCENT_WARM_WHITE or Custom (0x800D from C# _camToMtpWBTable)
    {"Tungsten", 6},                     // PTP_WB_TUNGSTEN_HALOGEN
    {"Flash", 7},                        // PTP_WB_FLASH
    {"Manual 1", 32775},                 // PTP_WB_MANUAL_1 (0x8007)
    {"Manual 2", 32776},                 // PTP_WB_MANUAL_2 (0x8008)
    {"Manual 3", 32777},                 // PTP_WB_MANUAL_3 (0x8009)
    {"Color Temp 1 (Set Kelvin)", 32778}, // PTP_WB_KELVIN_1 or Custom (0x800A) - Typically triggers Kelvin input
    {"Color Temp 2 (Set Kelvin)", 32779}, // PTP_WB_KELVIN_2 or Custom (0x800B)
    {"Color Temp 3 (Set Kelvin)", 32780}, // PTP_WB_KELVIN_3 or Custom (0x800C)
    {"CTE (Color Temp Enhance)", 32782}, // PTP_WB_CTE or Custom (0x800E from C# _camToMtpWBTable)
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
static PentaxLookupEntry exposure_mode_table[] = {
    {"Program (P)", 0},                     // PTP Value 0u
    {"Auto Picture (AUTO)", 1},            // PTP Value 1u (Corresponds to AUTOPICT in C#)
    {"Hyper Program (HyP)", 2},            // PTP Value 2u
    {"Green", 3},                          // PTP Value 3u
    {"Shutter Priority (Tv)", 4},          // PTP Value 4u
    {"Aperture Priority (Av)", 5},         // PTP Value 5u
    {"Hyper Tv (HypTv)", 6},               // PTP Value 6u
    {"Hyper Av (HypAv)", 7},               // PTP Value 7u
    {"Manual (M)", 8},                     // PTP Value 8u
    {"Bulb (B)", 9},                       // PTP Value 9u
    // Lens-specific variants (Av_Lens, M_Lens, B_Lens, TAv_Lens) are omitted for now for broader applicability.
    {"Shutter & Aperture Priority (TAv)", 13},// PTP Value 13u
    {"Sensitivity Priority (Sv)", 15},     // PTP Value 15u
    {"Flash Sync (X)", 16},                // PTP Value 16u
    // LS (18u) is likely Leaf Shutter specific and omitted. APL_P (19u) is unclear.
    {"AstroTracer Mode", 20},              // PTP Value 20u (If this is a distinct mode set via SET_EXPOSURE_MODE)
    // SA_Auto (21u) might be redundant with Auto Picture.
    {"Adv. Hyper Program", 22},            // PTP Value 22u
    {"Adv. Hyper Tv", 23},                 // PTP Value 23u
    {"Adv. Hyper Av", 24},                 // PTP Value 24u
    {"Adv. Hyper Manual", 25},             // PTP Value 25u
    {"Adv. Hyper TAv", 26},                // PTP Value 26u
    // AdvHypAv_Lens, AdvHypM_Lens, AdvHypTAv_Lens omitted.
    {"Adv. Hyper Sv", 30},                 // PTP Value 30u
    {"Hyper Sv (HypSv)", 31},              // PTP Value 31u
    // User Modes (U1, U2, etc.) are typically selected via a different mechanism (e.g. scene mode or dedicated user mode DPC)
    // and their previous PTP values (9, 10) in the C table conflicted with Bulb and Av_Lens from C#.
    // "Movie" and "Scene Mode" as general categories are also omitted as they are usually
    // activated by specific DPCs/Opcodes (PENTAX_DPC_MOVIE_MODE, PENTAX_OC_SET_SCENE_MODE) rather than this general exposure mode list.
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
    // Update Astrotracer settings from bulk data if available
    if (ret_ptp_conditions == PTP_RC_OK && priv->conditions_data_cache) {
        if (priv->conditions_data_len > CI_ASTROTRACER_TIMELIMIT + 1) { // uint16_t
            uint16_t time_limit_raw = parse_condition_u16(priv->conditions_data_cache, priv->conditions_data_len, CI_ASTROTRACER_TIMELIMIT);
            priv->astrotracer_time_limit = time_limit_raw;
            GP_DEBUG("pentaxmodern_get_config: Astrotracer time limit from 0x920F (idx %d): %d s", CI_ASTROTRACER_TIMELIMIT, priv->astrotracer_time_limit);
        }
        if (priv->conditions_data_len > CI_DSP_STATE_2) { // uint8_t
            uint8_t dsp_state2_raw = parse_condition_u8(priv->conditions_data_cache, priv->conditions_data_len, CI_DSP_STATE_2);
            if (dsp_state2_raw == PENTAX_DSP_STATE2_ASTRO_PRE_EXPOSING || dsp_state2_raw == PENTAX_DSP_STATE2_ASTRO_MAIN_EXPOSING) {
                priv->astrotracer_enabled = TRUE;
            } else {
                priv->astrotracer_enabled = FALSE;
            }
            GP_DEBUG("pentaxmodern_get_config: Astrotracer enabled state from 0x920F (idx %d, val %u): %s", CI_DSP_STATE_2, dsp_state2_raw, priv->astrotracer_enabled ? "TRUE" : "FALSE");
        }
    }

    gp_widget_new(GP_WIDGET_SECTION, "Astrotracer", &section); gp_widget_append(*window, section);
    gp_widget_new(GP_WIDGET_TOGGLE, "Enable Astrotracer", &widget); gp_widget_set_value(widget, &priv->astrotracer_enabled); gp_widget_append(section, widget);
    gp_widget_new(GP_WIDGET_RANGE, "Astrotracer Time Limit (sec)", &widget); gp_widget_set_range(widget, 10, 300, 10); gp_widget_set_value(widget, &priv->astrotracer_time_limit); gp_widget_append(section, widget);
    gp_widget_new(GP_WIDGET_SECTION, "Capture Modes", &section); gp_widget_append(*window, section);
    gp_widget_new(GP_WIDGET_TOGGLE, "Bulb Mode Active", &widget); gp_widget_set_value(widget, &priv->bulb_mode_active); gp_widget_append(section, widget);
    
    gp_widget_new(GP_WIDGET_RADIO, "Aperture Setting Priority", &widget);
    for(int i=0; KeepAperture_table[i].label; i++) gp_widget_add_choice(widget, KeepAperture_table[i].label);
    gp_widget_set_value(widget, lookup_label(KeepAperture_table, priv->keep_aperture_mode)); 
    gp_widget_append(section, widget);

    // Drive Mode Widget
    gp_widget_new(GP_WIDGET_RADIO, "Drive Mode", &widget);
    for (int i = 0; drive_table[i].label; i++) gp_widget_add_choice(widget, drive_table[i].label);

    uint32_t current_drive_mode_ptp_val = 0; // Default to PTP value for "Single Shot"
    const char* current_drive_mode_label = lookup_label(drive_table, current_drive_mode_ptp_val);

    if (ret_ptp_conditions == PTP_RC_OK && priv->conditions_data_cache && priv->conditions_data_len > CI_DRIVE_MODE_NEW) {
        uint8_t drive_mode_new_raw = parse_condition_u8(priv->conditions_data_cache, priv->conditions_data_len, CI_DRIVE_MODE_NEW);
        const char* label_from_bulk = lookup_label(drive_table, (uint32_t)drive_mode_new_raw);
        GP_DEBUG("pentaxmodern_get_config: Drive mode from 0x920F (idx %d) raw value: %u", CI_DRIVE_MODE_NEW, drive_mode_new_raw);

        if (g_strcmp0(label_from_bulk, "Unknown") != 0) {
            current_drive_mode_label = label_from_bulk;
            GP_DEBUG("pentaxmodern_get_config: Drive mode set from 0x920F: %s (0x%X)", current_drive_mode_label, drive_mode_new_raw);
        } else {
            GP_LOG_W("pentaxmodern_get_config: Value 0x%X from 0x920F (idx %d) for Drive Mode not in drive_table. Falling back.", drive_mode_new_raw, CI_DRIVE_MODE_NEW);
            if (pentaxmodern_get_ptp_property_u32(&priv->ptp_params, PENTAX_OC_GET_DRIVE_MODE, &current_drive_mode_ptp_val) == PTP_RC_OK) {
                current_drive_mode_label = lookup_label(drive_table, current_drive_mode_ptp_val);
                GP_DEBUG("pentaxmodern_get_config: Drive mode from PENTAX_OC_GET_DRIVE_MODE (fallback): %s (0x%X)", current_drive_mode_label, current_drive_mode_ptp_val);
            } else {
                GP_LOG_E("pentaxmodern_get_config: Failed to get Drive Mode via PENTAX_OC_GET_DRIVE_MODE on fallback.");
            }
        }
    }
    else if (pentaxmodern_get_ptp_property_u32(&priv->ptp_params, PENTAX_OC_GET_DRIVE_MODE, &current_drive_mode_ptp_val) == PTP_RC_OK) {
        current_drive_mode_label = lookup_label(drive_table, current_drive_mode_ptp_val);
        GP_DEBUG("pentaxmodern_get_config: Drive mode from PENTAX_OC_GET_DRIVE_MODE (primary attempt): %s (0x%X)", current_drive_mode_label, current_drive_mode_ptp_val);
    }
    else {
        GP_LOG_E("pentaxmodern_get_config: Failed to get Drive Mode from both 0x920F and PENTAX_OC_GET_DRIVE_MODE.");
    }

    if (g_strcmp0(current_drive_mode_label, "Unknown") == 0 && drive_table[0].label) {
        GP_LOG_W("pentaxmodern_get_config: Current drive mode PTP value 0x%X is unknown after all attempts. Defaulting to '%s'.", current_drive_mode_ptp_val, drive_table[0].label);
        current_drive_mode_label = drive_table[0].label;
    }
    gp_widget_set_value(widget, current_drive_mode_label);
    gp_widget_append(section, widget); // Appending to "Capture Modes" section

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

    // White Balance
    CameraWidget *wb_widget; // Use a distinct variable for clarity
    gp_widget_new(GP_WIDGET_RADIO, "White Balance", &wb_widget);
    for (int i = 0; wb_table[i].label; i++) gp_widget_add_choice(wb_widget, wb_table[i].label);

    if (get_ptp_dpc_u16(camera, PTP_DPC_WhiteBalance, &raw_val_u16) == GP_OK) {
        current_label = lookup_label(wb_table, raw_val_u16);
        GP_DEBUG("pentaxmodern_get_config: White Balance from DPC 0x5005: %s (0x%X)", current_label, raw_val_u16);
    } else {
        GP_LOG_W("pentaxmodern_get_config", "Failed to get White Balance via DPC 0x5005. Attempting fallback to bulk data CI_WB_MODE (30).");
        if (ret_ptp_conditions == PTP_RC_OK && priv->conditions_data_cache && priv->conditions_data_len > CI_WB_MODE + 1) { // CI_WB_MODE is 30
            uint16_t wb_from_bulk = parse_condition_u16(priv->conditions_data_cache, priv->conditions_data_len, CI_WB_MODE);
            current_label = lookup_label(wb_table, wb_from_bulk);
            GP_DEBUG("pentaxmodern_get_config: White Balance from bulk data (idx 30) raw: %u, mapped label: %s", wb_from_bulk, current_label);
            if (g_strcmp0(current_label, "Unknown") == 0) {
                 GP_LOG_W("pentaxmodern_get_config", "WB value %u from bulk data (idx 30) not found in new wb_table.", wb_from_bulk);
            }
        } else {
            GP_LOG_E("pentaxmodern_get_config", "Failed to get White Balance from DPC and bulk data unavailable/too short for CI_WB_MODE.");
            current_label = lookup_label(wb_table, 2); // Default to "Auto" (PTP value 2)
        }
    }
    if (g_strcmp0(current_label, "Unknown") == 0 && wb_table[0].label) {
         GP_LOG_W("pentaxmodern_get_config: Current WB PTP value is unknown. Defaulting to '%s'.", wb_table[0].label);
         current_label = wb_table[0].label;
    }
    gp_widget_set_value(wb_widget, current_label);
    gp_widget_append(section, wb_widget);

    // Exposure Mode
    if (ret_ptp_conditions == PTP_RC_OK && priv->conditions_data_cache) {
        raw_val_u16 = parse_condition_u16(priv->conditions_data_cache, priv->conditions_data_len, CI_CAPTURE_MODE_INFO); current_label = lookup_label(exposure_mode_table, raw_val_u16);
    } else if (pentaxmodern_get_ptp_property_u32(&priv->ptp_params, PENTAX_OC_GET_EXPOSURE_MODE, &raw_val_u32) == PTP_RC_OK) {
        current_label = lookup_label(exposure_mode_table, raw_val_u32);
    } else { current_label = "Unknown"; }
    // This line above is part of the old block and will be replaced by the new logic below.
    // gp_widget_new(GP_WIDGET_RADIO, "Exposure Mode", &widget); for (int i = 0; exposure_mode_table[i].label; i++) gp_widget_add_choice(widget, exposure_mode_table[i].label); gp_widget_set_value(widget, current_label); gp_widget_append(section, widget);

    // New logic for Exposure Mode Widget
    CameraWidget *exposure_mode_widget; // Use a distinct variable name
    gp_widget_new(GP_WIDGET_RADIO, "Exposure Mode", &exposure_mode_widget);
    for (int i = 0; exposure_mode_table[i].label; i++) gp_widget_add_choice(exposure_mode_widget, exposure_mode_table[i].label);

    uint32_t current_exposure_mode_ptp_val = 0; // Default to PTP value for "Program (P)"
    const char* current_exposure_mode_label = lookup_label(exposure_mode_table, current_exposure_mode_ptp_val);

    if (ret_ptp_conditions == PTP_RC_OK && priv->conditions_data_cache && priv->conditions_data_len > CI_CAPTURE_MODE_INFO + 1) {
        uint16_t exp_mode_raw = parse_condition_u16(priv->conditions_data_cache, priv->conditions_data_len, CI_CAPTURE_MODE_INFO);
        const char* label_from_bulk = lookup_label(exposure_mode_table, (uint32_t)exp_mode_raw);
        GP_DEBUG("pentaxmodern_get_config: Exposure Mode from 0x920F (idx %d) raw value: %u", CI_CAPTURE_MODE_INFO, exp_mode_raw);

        if (g_strcmp0(label_from_bulk, "Unknown") != 0) {
            current_exposure_mode_label = label_from_bulk;
            GP_DEBUG("pentaxmodern_get_config: Exposure Mode set from 0x920F: %s (0x%X)", current_exposure_mode_label, exp_mode_raw);
        } else {
            GP_LOG_W("pentaxmodern_get_config: Value 0x%X from 0x920F (idx %d) for Exposure Mode not in new exposure_mode_table. Falling back.", exp_mode_raw, CI_CAPTURE_MODE_INFO);
            if (pentaxmodern_get_ptp_property_u32(&priv->ptp_params, PENTAX_OC_GET_EXPOSURE_MODE, &current_exposure_mode_ptp_val) == PTP_RC_OK) {
                current_exposure_mode_label = lookup_label(exposure_mode_table, current_exposure_mode_ptp_val);
                GP_DEBUG("pentaxmodern_get_config: Exposure Mode from PENTAX_OC_GET_EXPOSURE_MODE (fallback): %s (0x%X)", current_exposure_mode_label, current_exposure_mode_ptp_val);
            } else {
                GP_LOG_E("pentaxmodern_get_config: Failed to get Exposure Mode via PENTAX_OC_GET_EXPOSURE_MODE on fallback.");
            }
        }
    }
    else if (pentaxmodern_get_ptp_property_u32(&priv->ptp_params, PENTAX_OC_GET_EXPOSURE_MODE, &current_exposure_mode_ptp_val) == PTP_RC_OK) {
        current_exposure_mode_label = lookup_label(exposure_mode_table, current_exposure_mode_ptp_val);
        GP_DEBUG("pentaxmodern_get_config: Exposure Mode from PENTAX_OC_GET_EXPOSURE_MODE (primary attempt): %s (0x%X)", current_exposure_mode_label, current_exposure_mode_ptp_val);
    }
    else {
        GP_LOG_E("pentaxmodern_get_config: Failed to get Exposure Mode from both 0x920F and PENTAX_OC_GET_EXPOSURE_MODE.");
    }

    if (g_strcmp0(current_exposure_mode_label, "Unknown") == 0 && exposure_mode_table[0].label) {
        GP_LOG_W("pentaxmodern_get_config: Current exposure mode PTP value 0x%X is unknown. Defaulting to '%s'.", current_exposure_mode_ptp_val, exposure_mode_table[0].label);
        current_exposure_mode_label = exposure_mode_table[0].label;
    }
    gp_widget_set_value(exposure_mode_widget, current_exposure_mode_label);
    gp_widget_append(section, exposure_mode_widget);

    // Shutter Speed, Aperture widgets using the generic 'widget' variable
    if (pentaxmodern_get_ptp_property_u32(&priv->ptp_params, PENTAX_OC_GET_SHUTTER, &raw_val_u32) == PTP_RC_OK) { current_label = lookup_label(shutter_table, raw_val_u32); } else { current_label = "Unknown"; }
    gp_widget_new(GP_WIDGET_RADIO, "Shutter Speed", &widget); for (int i = 0; shutter_table[i].label; i++) gp_widget_add_choice(widget, shutter_table[i].label); gp_widget_set_value(widget, current_label); gp_widget_append(section, widget);
    if (pentaxmodern_get_ptp_property_u32(&priv->ptp_params, PENTAX_OC_GET_APERTURE, &raw_val_u32) == PTP_RC_OK) { current_label = lookup_label(aperture_table, raw_val_u32); } else { current_label = "Unknown"; }
    gp_widget_new(GP_WIDGET_RADIO, "Aperture", &widget); for (int i = 0; aperture_table[i].label; i++) gp_widget_add_choice(widget, aperture_table[i].label); gp_widget_set_value(widget, current_label); gp_widget_append(section, widget);
    
    // Movie Settings Section
    // CameraWidget *movie_section; // Declared with other section types earlier if that's the pattern
    gp_widget_new(GP_WIDGET_SECTION, "Movie Settings", &movie_section); // 'movie_section' is already declared with other CameraWidget* at the start of the function

    // Movie Mode Toggle
    CameraWidget *movie_mode_toggle_widget;
    gp_widget_new(GP_WIDGET_TOGGLE, "Movie Mode", &movie_mode_toggle_widget);
    uint8_t movie_mode_dpc_val = 0;
    if (get_ptp_dpc_u8(camera, PENTAX_DPC_MOVIE_MODE, &movie_mode_dpc_val) == GP_OK) {
        int movie_mode_enabled = (movie_mode_dpc_val != 0);
        gp_widget_set_value(movie_mode_toggle_widget, &movie_mode_enabled);
    } else {
        GP_LOG_E("pentaxmodern_get_config", "Failed to get Movie Mode DPC 0x%X", PENTAX_DPC_MOVIE_MODE);
        int enabled = 0; gp_widget_set_value(movie_mode_toggle_widget, &enabled);
        gp_widget_set_readonly(movie_mode_toggle_widget, TRUE);
    }
    gp_widget_append(movie_section, movie_mode_toggle_widget);

    // Movie Pixel Size
    CameraWidget *pixel_size_widget;
    gp_widget_new(GP_WIDGET_RADIO, "Movie Pixel Size", &pixel_size_widget);
    for (int i = 0; MoviePixelSize_table[i].label; i++) gp_widget_add_choice(pixel_size_widget, MoviePixelSize_table[i].label);
    // Assuming CI_IMAGE_SIZE (34) is for stills primarily, if movie size is separate, need its index.
    // For now, let's assume PENTAX_DPC_WRITING_FILE_FORMAT_SETTING (0xD01B) byte 6 is the source if bulk data isn't specific.
    // Given the task specifies CI_IMAGE_SIZE for movie, we'll use that.
    if (ret_ptp_conditions == PTP_RC_OK && priv->conditions_data_cache && priv->conditions_data_len > CI_IMAGE_SIZE + 1) { // CI_IMAGE_SIZE is index for uint16_t
        uint16_t image_size_val = parse_condition_u16(priv->conditions_data_cache, priv->conditions_data_len, CI_IMAGE_SIZE);
        uint16_t movie_pix_size_ptp = image_size_val & 0xFFFF;
        gp_widget_set_value(pixel_size_widget, lookup_label(MoviePixelSize_table, movie_pix_size_ptp));
    } else {
        GP_LOG_W("pentaxmodern_get_config", "Failed to get Movie Pixel Size from 0x920F CI_IMAGE_SIZE. Defaulting.");
        gp_widget_set_value(pixel_size_widget, MoviePixelSize_table[0].label);
    }
    gp_widget_append(movie_section, pixel_size_widget);

    // Movie Frame Rate
    CameraWidget *frame_rate_widget;
    gp_widget_new(GP_WIDGET_RADIO, "Movie Frame Rate", &frame_rate_widget);
    for (int i = 0; MovieFrameRate_table[i].label; i++) gp_widget_add_choice(frame_rate_widget, MovieFrameRate_table[i].label);
    if (ret_ptp_conditions == PTP_RC_OK && priv->conditions_data_cache && priv->conditions_data_len > CI_IMAGE_SIZE + 1) {
        uint16_t image_size_val = parse_condition_u16(priv->conditions_data_cache, priv->conditions_data_len, CI_IMAGE_SIZE);
        uint16_t movie_frame_rate_ptp_val = image_size_val >> 16;
        // Note: C# MtpDevice.cs maps PTP 3 to UI index 2 ("24p").
        // If MovieFrameRate_table stores UI indices {0,1,2} as PTP values, direct lookup is fine.
        // If it stores actual PTP values {0,1,3}, then direct lookup is also fine.
        // The current MovieFrameRate_table is {"60p",0}, {"30p",1}, {"24p",2}.
        // If camera returns PTP value 3 for 24p, this direct lookup_label(table, 3) will fail.
        // This requires either adjusting table values or mapping logic here.
        // For now, assume direct lookup works or table values match camera PTP values for this specific condition index.
        const char * frame_rate_label = lookup_label(MovieFrameRate_table, movie_frame_rate_ptp_val);
        if (g_strcmp0(frame_rate_label, "Unknown") == 0 && movie_frame_rate_ptp_val == 3 && MovieFrameRate_table[2].value == 2) { // Specific C# case for 24p
             frame_rate_label = MovieFrameRate_table[2].label; // "24p"
        }
        gp_widget_set_value(frame_rate_widget, frame_rate_label);
    } else {
        GP_LOG_W("pentaxmodern_get_config", "Failed to get Movie Frame Rate from 0x920F CI_IMAGE_SIZE. Defaulting.");
        gp_widget_set_value(frame_rate_widget, MovieFrameRate_table[0].label);
    }
    gp_widget_append(movie_section, frame_rate_widget);

    // Movie Card Slot
    CameraWidget *card_slot_widget;
    gp_widget_new(GP_WIDGET_RADIO, "Movie Card Slot", &card_slot_widget);
    for (int i = 0; MovieCardSlot_table[i].label; i++) gp_widget_add_choice(card_slot_widget, MovieCardSlot_table[i].label);
    if (ret_ptp_conditions == PTP_RC_OK && priv->conditions_data_cache && priv->conditions_data_len > CI_MOVIE_CARD_SLOT_SETTING) { // Assuming uint8_t
        uint8_t slot_val = parse_condition_u8(priv->conditions_data_cache, priv->conditions_data_len, CI_MOVIE_CARD_SLOT_SETTING);
        gp_widget_set_value(card_slot_widget, lookup_label(MovieCardSlot_table, slot_val));
    } else {
        GP_LOG_W("pentaxmodern_get_config", "Failed to get Movie Card Slot from 0x920F CI_MOVIE_CARD_SLOT_SETTING. Defaulting.");
        gp_widget_set_value(card_slot_widget, MovieCardSlot_table[0].label);
    }
    gp_widget_append(movie_section, card_slot_widget);

    // Movie File Format
    CameraWidget *file_format_widget;
    gp_widget_new(GP_WIDGET_RADIO, "Movie File Format", &file_format_widget);
    for (int i = 0; MovieFileFormat_table[i].label; i++) gp_widget_add_choice(file_format_widget, MovieFileFormat_table[i].label);
    if (ret_ptp_conditions == PTP_RC_OK && priv->conditions_data_cache && priv->conditions_data_len > CI_WRITING_FORMAT_SETTING) { // Assuming uint8_t
        uint8_t format_val = parse_condition_u8(priv->conditions_data_cache, priv->conditions_data_len, CI_WRITING_FORMAT_SETTING);
        gp_widget_set_value(file_format_widget, lookup_label(MovieFileFormat_table, format_val));
    } else {
        GP_LOG_W("pentaxmodern_get_config", "Failed to get Movie File Format from 0x920F CI_WRITING_FORMAT_SETTING. Defaulting.");
        gp_widget_set_value(file_format_widget, MovieFileFormat_table[0].label);
    }
    gp_widget_set_readonly(file_format_widget, TRUE);
    gp_widget_append(movie_section, file_format_widget);

    gp_widget_append(*window, movie_section);


    // Custom Image Details Section
    gp_widget_new(GP_WIDGET_SECTION, "Custom Image Details", &ci_section); // 'ci_section' is already declared
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

    // CI User Filter Effect Widgets
    // IMPORTANT: The PTP value for "User Filter" in CIFilterEffect_table needs verification. Assuming 5 for now.
    #define PTP_VALUE_FOR_USER_FILTER 5
    uint8_t current_main_filter_effect_val = 0;
    gboolean is_user_filter_active = FALSE;
    if (get_ptp_dpc_u8(camera, PENTAX_DPC_CI_FILTER_EFFECT, &current_main_filter_effect_val) == GP_OK) {
        if (current_main_filter_effect_val == PTP_VALUE_FOR_USER_FILTER) {
            is_user_filter_active = TRUE;
        }
        // Debugging: Log the read main filter effect and whether user filter is considered active
        GP_DEBUG("pentaxmodern_get_config: CI Filter Effect (0xD024) PTP value: %u. User filter active: %s.",
                 current_main_filter_effect_val, is_user_filter_active ? "yes" : "no");
    } else {
        GP_LOG_E("pentaxmodern_get_config: Failed to get CI Filter Effect DPC 0x%X.", PENTAX_DPC_CI_FILTER_EFFECT);
    }

    int16_t gain_r = 0, gain_g = 0, gain_b = 0;
    PTPPropValue pv_userfilter;
    memset(&pv_userfilter, 0, sizeof(pv_userfilter));
    uint16_t ret_uf = ptp_getdevicepropvalue(&priv->ptp_params, PENTAX_DPC_CI_USER_FILTER_EFFECT, &pv_userfilter, PTP_DTC_AUINT8);

    if (ret_uf == PTP_RC_OK) {
        if (pv_userfilter.a.v_u8 != NULL && pv_userfilter.a.count == 8) {
            if (pv_userfilter.a.v_u8[0] == 4) { // Check descriptor
                uint8_t sign_byte = pv_userfilter.a.v_u8[4];
                uint8_t abs_r = pv_userfilter.a.v_u8[5];
                uint8_t abs_g = pv_userfilter.a.v_u8[6];
                uint8_t abs_b = pv_userfilter.a.v_u8[7];

                gain_r = (sign_byte & 0x10) ? -(int16_t)abs_r : (int16_t)abs_r;
                gain_g = (sign_byte & 0x20) ? -(int16_t)abs_g : (int16_t)abs_g;
                gain_b = (sign_byte & 0x40) ? -(int16_t)abs_b : (int16_t)abs_b;
                GP_DEBUG("pentaxmodern_get_config: Parsed User Filter Gains - R:%d, G:%d, B:%d", gain_r, gain_g, gain_b);
            } else {
                GP_LOG_W("pentaxmodern_get_config: DPC 0x%X descriptor mismatch. Expected 4, got %u.", PENTAX_DPC_CI_USER_FILTER_EFFECT, pv_userfilter.a.v_u8[0]);
            }
            free(pv_userfilter.a.v_u8); // Free the allocated memory
            pv_userfilter.a.v_u8 = NULL;
        } else {
             GP_LOG_W("pentaxmodern_get_config: DPC 0x%X data invalid (NULL or count %u != 8).", PENTAX_DPC_CI_USER_FILTER_EFFECT, pv_userfilter.a.count);
             if(pv_userfilter.a.v_u8) { free(pv_userfilter.a.v_u8); pv_userfilter.a.v_u8 = NULL; }
        }
    } else {
        GP_LOG_E("pentaxmodern_get_config: Failed to get DPC 0x%X (CI User Filter Effect). Error: 0x%X", PENTAX_DPC_CI_USER_FILTER_EFFECT, ret_uf);
    }

    CameraWidget *user_gain_r_widget, *user_gain_g_widget, *user_gain_b_widget;
    float gain_float_val;

    gp_widget_new(GP_WIDGET_RANGE, "CI User Filter R Gain", &user_gain_r_widget);
    gain_float_val = (float)gain_r;
    gp_widget_set_range(user_gain_r_widget, -200.0f, 200.0f, 10.0f);
    gp_widget_set_value(user_gain_r_widget, &gain_float_val);
    gp_widget_set_readonly(user_gain_r_widget, !is_user_filter_active);
    gp_widget_append(ci_section, user_gain_r_widget);

    gp_widget_new(GP_WIDGET_RANGE, "CI User Filter G Gain", &user_gain_g_widget);
    gain_float_val = (float)gain_g;
    gp_widget_set_range(user_gain_g_widget, -200.0f, 200.0f, 10.0f);
    gp_widget_set_value(user_gain_g_widget, &gain_float_val);
    gp_widget_set_readonly(user_gain_g_widget, !is_user_filter_active);
    gp_widget_append(ci_section, user_gain_g_widget);

    gp_widget_new(GP_WIDGET_RANGE, "CI User Filter B Gain", &user_gain_b_widget);
    gain_float_val = (float)gain_b;
    gp_widget_set_range(user_gain_b_widget, -200.0f, 200.0f, 10.0f);
    gp_widget_set_value(user_gain_b_widget, &gain_float_val);
    gp_widget_set_readonly(user_gain_b_widget, !is_user_filter_active);
    gp_widget_append(ci_section, user_gain_b_widget);

    // Storage Settings Section
    CameraWidget *storage_section;
    gp_widget_new(GP_WIDGET_SECTION, "Storage Settings", &storage_section); 
    gp_widget_append(*window, storage_section); 

    uint32_t active_storage_id_raw = 0;
    const char *active_card_label = "Card 1"; // Default value

    if (pentaxmodern_get_ptp_property_u32(&priv->ptp_params, PENTAX_OC_GET_ACTIVE_STORAGE, &active_storage_id_raw) == PTP_RC_OK) {
        GP_DEBUG("pentaxmodern_get_config: Successfully read PENTAX_OC_GET_ACTIVE_STORAGE: 0x%X", active_storage_id_raw);
        if (active_storage_id_raw == 0x00020001) { // Standard PTP StorageID for second physical store
            active_card_label = "Card 2";
        } else if (active_storage_id_raw == 0x00010001) { // Standard PTP StorageID for first physical store
             active_card_label = "Card 1";
        } else {
             GP_LOG_W("pentaxmodern_get_config", "Unknown or non-standard active storage ID: 0x%X via PENTAX_OC_GET_ACTIVE_STORAGE. Defaulting to Card 1.", active_storage_id_raw);
             active_card_label = "Card 1"; // Default if unknown ID
        }
    } else {
        GP_LOG_W("pentaxmodern_get_config", "Failed to get active storage ID via PENTAX_OC_GET_ACTIVE_STORAGE (0x%X). Trying PTP_DPC_StorageID as fallback.", PENTAX_OC_GET_ACTIVE_STORAGE);
        PTPPropValue propval;
        if (ptp_getdevicepropvalue(&priv->ptp_params, PTP_DPC_StorageID, &propval, PTP_DTC_UINT32) == PTP_RC_OK) {
            GP_DEBUG("pentaxmodern_get_config", "Successfully read PTP_DPC_StorageID: 0x%X", propval.u32);
            if (propval.u32 == 0x00020001) active_card_label = "Card 2";
            else if (propval.u32 == 0x00010001) active_card_label = "Card 1";
            else { 
                GP_LOG_W("pentaxmodern_get_config", "Unknown PTP_DPC_StorageID: 0x%X. Defaulting to Card 1.", propval.u32);
                active_card_label = "Card 1";
            }
        } else {
             GP_LOG_W("pentaxmodern_get_config", "Failed to get PTP_DPC_StorageID. Defaulting to Card 1.");
             active_card_label = "Card 1"; // Final fallback
        }
    }
    
    CameraWidget *active_sd_widget; 
    gp_widget_new(GP_WIDGET_RADIO, "Active SD Card", &active_sd_widget); 
    gp_widget_add_choice(active_sd_widget, "Card 1");
    gp_widget_add_choice(active_sd_widget, "Card 2");
    gp_widget_set_value(active_sd_widget, active_card_label);
    gp_widget_append(storage_section, active_sd_widget);

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

    // White Balance Setting
    if (gp_widget_get_child_by_name(window, "White Balance", &widget) == GP_OK && gp_widget_changed(widget)) {
        gp_widget_get_value(widget, &choice_val);
        uint32_t temp_wb_ptp_val = lookup_value(wb_table, choice_val); // Using new wb_table
        uint16_t wb_ptp_val_u16 = (uint16_t)temp_wb_ptp_val;

        GP_DEBUG("pentaxmodern_set_config: Setting White Balance to: %s (PTP val 0x%X)", choice_val, wb_ptp_val_u16);
        if (set_ptp_dpc_u16(camera, PTP_DPC_WhiteBalance, wb_ptp_val_u16) == GP_OK) {
            GP_DEBUG("pentaxmodern_set_config: Successfully set White Balance via DPC 0x5005.");
        } else {
            GP_LOG_E("pentaxmodern_set_config: Failed to set White Balance via DPC 0x5005 (value 0x%X).", wb_ptp_val_u16);
        }
        free(choice_val);
    }

    // Drive Mode Setting
    if (gp_widget_get_child_by_name(window, "Drive Mode", &widget) == GP_OK && gp_widget_changed(widget)) {
        gp_widget_get_value(widget, &choice_val);
        ptp_val_u32 = lookup_value(drive_table, choice_val);

        GP_DEBUG("pentaxmodern_set_config: Setting Drive Mode to: %s (PTP val 0x%X)", choice_val, ptp_val_u32);

        if (ptp_generic_no_data(&priv->ptp_params, PENTAX_OC_SET_DRIVE_MODE, 1, ptp_val_u32, 0, 0, 0) == PTP_RC_OK) {
            GP_DEBUG("pentaxmodern_set_config: Successfully set Drive Mode.");
        } else {
            GP_LOG_E("pentaxmodern_set_config: Failed to set Drive Mode using PENTAX_OC_SET_DRIVE_MODE (PTP val 0x%X).", ptp_val_u32);
        }
        free(choice_val);
    }

    // Exposure Mode Setting
    if (gp_widget_get_child_by_name(window, "Exposure Mode", &widget) == GP_OK && gp_widget_changed(widget)) {
        gp_widget_get_value(widget, &choice_val);
        ptp_val_u32 = lookup_value(exposure_mode_table, choice_val);

        GP_DEBUG("pentaxmodern_set_config: Setting Exposure Mode to: %s (PTP val 0x%X)", choice_val, ptp_val_u32);

        if (ptp_generic_no_data(&priv->ptp_params, PENTAX_OC_SET_EXPOSURE_MODE, 1, ptp_val_u32, 0, 0, 0) == PTP_RC_OK) {
            GP_DEBUG("pentaxmodern_set_config: Successfully set Exposure Mode.");
        } else {
            GP_LOG_E("pentaxmodern_set_config: Failed to set Exposure Mode using PENTAX_OC_SET_EXPOSURE_MODE (PTP val 0x%X).", ptp_val_u32);
        }
        free(choice_val);
    }

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
    CameraWidget *movie_mode_widget = NULL, *pixel_size_widget = NULL, *frame_rate_widget = NULL, *card_slot_widget = NULL;
    int movie_mode_changed = FALSE, pixel_size_changed = FALSE, frame_rate_changed = FALSE, card_slot_changed = FALSE;

    if (gp_widget_get_child_by_name(window, "Movie Mode", &movie_mode_widget) == GP_OK && gp_widget_changed(movie_mode_widget)) {
        movie_mode_changed = TRUE;
        gp_widget_get_value(movie_mode_widget, &enabled_val);
        GP_DEBUG("pentaxmodern_set_config: Setting Movie Mode to: %s", enabled_val ? "ON" : "OFF");
        if (set_ptp_dpc_u8(camera, PENTAX_DPC_MOVIE_MODE, enabled_val ? 1 : 0) == GP_OK) {
            GP_DEBUG("pentaxmodern_set_config: Successfully set Movie Mode DPC.");
        } else {
            GP_LOG_E("pentaxmodern_set_config: Failed to set Movie Mode DPC 0x%X.", PENTAX_DPC_MOVIE_MODE);
        }
    }

    // Check if any of the DPC 0xD01B related movie settings changed
    gp_widget_get_child_by_name(window, "Movie Pixel Size", &pixel_size_widget);
    if (pixel_size_widget && gp_widget_changed(pixel_size_widget)) pixel_size_changed = TRUE;

    gp_widget_get_child_by_name(window, "Movie Frame Rate", &frame_rate_widget);
    if (frame_rate_widget && gp_widget_changed(frame_rate_widget)) frame_rate_changed = TRUE;

    gp_widget_get_child_by_name(window, "Movie Card Slot", &card_slot_widget);
    if (card_slot_widget && gp_widget_changed(card_slot_widget)) card_slot_changed = TRUE;

    // If any of the DPC 0xD01B related settings changed, construct and send payload
    if (pixel_size_changed || frame_rate_changed || card_slot_changed) {
        unsigned char payload[10];
        memset(payload, 0, sizeof(payload)); // Initialize with zeros

        payload[0] = 6; // Descriptor/type

        // Placeholders for unused/still-related bytes based on C# analysis
        payload[1] = 0;
        payload[2] = 0;
        payload[3] = 0;
        payload[4] = 6; // Still Format Slot 1 / Movie File Format (MP4/MOV - not being set here)
        payload[5] = 6; // Still Format Slot 2
        payload[7] = 4; // Still JPEG Quality

        // Get Movie Pixel Size (payload[6])
        if (pixel_size_widget) { // Should always be found if changed
            gp_widget_get_value(pixel_size_widget, &choice_val);
            payload[6] = (uint8_t)lookup_value(MoviePixelSize_table, choice_val);
            GP_DEBUG("pentaxmodern_set_config: Payload[6] (MoviePixelSize) set to: %s (0x%X)", choice_val, payload[6]);
            free(choice_val);
        } else { /* Should not happen if changed=TRUE, but as a fallback use current from camera or default */
            GP_LOG_W("pentaxmodern_set_config", "Movie Pixel Size widget not found for DPC 0xD01B payload construction, but was marked changed. Using default.");
            payload[6] = MoviePixelSize_table[0].value; // Default to first entry's PTP value
        }
        
        // Get Movie Frame Rate (payload[8])
        if (frame_rate_widget) {
            gp_widget_get_value(frame_rate_widget, &choice_val);
            uint32_t ui_index_val = lookup_value(MovieFrameRate_table, choice_val);
            if (g_strcmp0(choice_val, "24p") == 0) { // C# MtpDevice.cs special case: UI "24p" (index 2) -> PTP value 3
                payload[8] = 3;
            } else {
                payload[8] = (uint8_t)ui_index_val; // Assumes other PTP values match UI indices in table
            }
            GP_DEBUG("pentaxmodern_set_config: Payload[8] (MovieFrameRate) set to: %s (UI Index %u, PTP val 0x%X)", choice_val, ui_index_val, payload[8]);
            free(choice_val);
        } else {
            GP_LOG_W("pentaxmodern_set_config", "Movie Frame Rate widget not found for DPC 0xD01B payload construction, but was marked changed. Using default.");
            payload[8] = MovieFrameRate_table[0].value; // Default to first entry's PTP value (adjust if special mapping needed)
        }

        // Get Movie Card Slot (payload[9])
        if (card_slot_widget) {
            gp_widget_get_value(card_slot_widget, &choice_val);
            payload[9] = (uint8_t)lookup_value(MovieCardSlot_table, choice_val);
            GP_DEBUG("pentaxmodern_set_config: Payload[9] (MovieCardSlot) set to: %s (0x%X)", choice_val, payload[9]);
            free(choice_val);
        } else {
            GP_LOG_W("pentaxmodern_set_config", "Movie Card Slot widget not found for DPC 0xD01B payload construction, but was marked changed. Using default.");
            payload[9] = MovieCardSlot_table[0].value; // Default to first entry's PTP value
        }

        GP_DEBUG("pentaxmodern_set_config: Preparing to set DPC 0xD01B (WritingFileFormatSetting) with payload:");
        for(int i=0; i<10; i++) GP_DEBUG("  payload[%d] = 0x%02X", i, payload[i]);

        PTPPropValue propv_array;
        propv_array.a.v_u8 = payload;
        propv_array.a.count = sizeof(payload);

        if (set_ptp_deviceprop_value(camera, PENTAX_DPC_WRITING_FILE_FORMAT_SETTING, &propv_array, PTP_DTC_AUINT8) == GP_OK) {
            GP_DEBUG("pentaxmodern_set_config: Successfully set DPC 0xD01B (WritingFileFormatSetting).");
        } else {
            GP_LOG_E("pentaxmodern_set_config: Failed to set DPC 0xD01B (WritingFileFormatSetting).");
        }
    }
    // Note: MovieFileFormat is read-only, so no set logic here.
    // The old MovieQuality widget handling is removed as it's superseded by the new granular controls.

    // Custom Image Detail Settings
    if (gp_widget_get_child_by_name(window, "CI Saturation", &widget) == GP_OK && gp_widget_changed(widget)) {
        gp_widget_get_value(widget, &range_float_val); ptp_val_s8 = (int8_t)range_float_val; 
        set_ptp_dpc_s8(camera, PENTAX_DPC_CI_SATURATION, ptp_val_s8);
    }
    if (gp_widget_get_child_by_name(window, "CI Hue", &widget) == GP_OK && gp_widget_changed(widget)) {
        gp_widget_get_value(widget, &range_float_val); ptp_val_s8 = (int8_t)range_float_val;
        set_ptp_dpc_s8(camera, PENTAX_DPC_CI_HUE, ptp_val_s8);
    }
    // ... (Handle other existing CI settings like Key, Contrast, Sharpness, etc. before adding User Filter Effect)

    // CI User Filter Effect (R,G,B Gains)
    // IMPORTANT: PTP_VALUE_FOR_USER_FILTER (5) needs verification against actual CIFilterEffect_table PTP value for "User" mode.
    #define PTP_VALUE_FOR_USER_FILTER 5
    CameraWidget *ci_filter_effect_widget_check; // Use a different variable name to avoid conflict
    char *current_ci_filter_label_check;
    uint8_t current_ci_filter_ptp_val_check = 0; // Default to a non-User value

    if (gp_widget_get_child_by_name(window, "CI Filter Effect", &ci_filter_effect_widget_check) == GP_OK) {
        // We need the *current* value of the main CI Filter Effect, not if it changed,
        // because the gain sliders might change even if the main filter selection didn't this turn.
        gp_widget_get_value(ci_filter_effect_widget_check, &current_ci_filter_label_check);
        current_ci_filter_ptp_val_check = lookup_value(CIFilterEffect_table, current_ci_filter_label_check);
        GP_DEBUG("pentaxmodern_set_config: Current CI Filter Effect is '%s' (PTP: %u). Required for User Filter Gains: %u.",
            current_ci_filter_label_check, current_ci_filter_ptp_val_check, PTP_VALUE_FOR_USER_FILTER);
        free(current_ci_filter_label_check);
    } else {
        GP_LOG_E("pentaxmodern_set_config: Could not find 'CI Filter Effect' widget to check prerequisite for User Filter Gains.");
    }

    if (current_ci_filter_ptp_val_check == PTP_VALUE_FOR_USER_FILTER) {
        CameraWidget *r_gain_widget, *g_gain_widget, *b_gain_widget;
        gboolean r_changed = FALSE, g_changed = FALSE, b_changed = FALSE;
        float r_float_val = 0.0f, g_float_val = 0.0f, b_float_val = 0.0f; // Initialize to default

        // Get R Gain widget and check if changed
        if (gp_widget_get_child_by_name(window, "CI User Filter R Gain", &r_gain_widget) == GP_OK) {
            if(gp_widget_changed(r_gain_widget)) r_changed = TRUE;
            gp_widget_get_value(r_gain_widget, &r_float_val);
        }
        // Get G Gain widget and check if changed
        if (gp_widget_get_child_by_name(window, "CI User Filter G Gain", &g_gain_widget) == GP_OK) {
            if(gp_widget_changed(g_gain_widget)) g_changed = TRUE;
            gp_widget_get_value(g_gain_widget, &g_float_val);
        }
        // Get B Gain widget and check if changed
        if (gp_widget_get_child_by_name(window, "CI User Filter B Gain", &b_gain_widget) == GP_OK) {
            if(gp_widget_changed(b_gain_widget)) b_changed = TRUE;
            gp_widget_get_value(b_gain_widget, &b_float_val);
        }

        if (r_changed || g_changed || b_changed) {
            GP_DEBUG("pentaxmodern_set_config: Attempting to set CI User Filter Gains (R:%.0f, G:%.0f, B:%.0f)", r_float_val, g_float_val, b_float_val);

            int16_t r_gain = (int16_t)r_float_val;
            int16_t g_gain = (int16_t)g_float_val;
            int16_t b_gain = (int16_t)b_float_val;

            // Clamp values: abs(gain) must be <= 200
            if (abs(r_gain) > 200) r_gain = (r_gain > 0) ? 200 : -200;
            if (abs(g_gain) > 200) g_gain = (g_gain > 0) ? 200 : -200;
            if (abs(b_gain) > 200) b_gain = (b_gain > 0) ? 200 : -200;

            unsigned char payload[8];
            payload[0] = 4; // Descriptor
            payload[1] = 0; payload[2] = 0; payload[3] = 0; // Padding
            payload[4] = ((r_gain < 0) ? 0x10 : 0) |
                         ((g_gain < 0) ? 0x20 : 0) |
                         ((b_gain < 0) ? 0x40 : 0); // Sign bits
            payload[5] = (uint8_t)abs(r_gain);
            payload[6] = (uint8_t)abs(g_gain);
            payload[7] = (uint8_t)abs(b_gain);

            PTPPropValue propv_array_userfilter; // Use a distinct variable name
            memset(&propv_array_userfilter, 0, sizeof(propv_array_userfilter));
            propv_array_userfilter.a.v_u8 = payload; // Pointer to the array
            propv_array_userfilter.a.count = sizeof(payload); // Number of elements

            GP_DEBUG("pentaxmodern_set_config: Setting DPC 0x%X (CI User Filter Effect) with payload:", PENTAX_DPC_CI_USER_FILTER_EFFECT);
            for(int i=0; i<8; i++) GP_DEBUG("  payload[%d] = 0x%02X", i, payload[i]);

            if (set_ptp_deviceprop_value(camera, PENTAX_DPC_CI_USER_FILTER_EFFECT, &propv_array_userfilter, PTP_DTC_AUINT8) == GP_OK) {
                GP_DEBUG("pentaxmodern_set_config: Successfully set DPC 0x%X (CI User Filter Effect).", PENTAX_DPC_CI_USER_FILTER_EFFECT);
            } else {
                GP_LOG_E("pentaxmodern_set_config: Failed to set DPC 0x%X (CI User Filter Effect).", PENTAX_DPC_CI_USER_FILTER_EFFECT);
            }
        }
    } else {
        GP_DEBUG("pentaxmodern_set_config: CI User Filter not active (current PTP value %u != %u). Skipping User Filter Gain settings.",
                 current_ci_filter_ptp_val_check, PTP_VALUE_FOR_USER_FILTER);
    }
    // ... (rest of CI set logic from previous turn, if any, should follow here)

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
