/* Descriptive UVC identity table only; this is not a libgphoto2 UVC camlib.
 * Keep vendor-protocol cameras out of this table. The measured Orion
 * StarShoot All-in-One (16c0:29a0) is USB class 255, not UVC class 14. */
static const struct uvc_device {
    uint16_t vid, pid;
    const char *vendor, *model;
} uvc_devices[] = {
    {0x1233, 0x1455, "iOptron", "iPolar"},
    {0, 0, NULL, NULL}
};
