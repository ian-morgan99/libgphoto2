/* UVC device list - Orion Starshoot All-in-One (QHY5L-II clone) */
static const struct uvc_device {
    uint16_t vid, pid;
    const char *vendor, *model;
} uvc_devices[] = {
    {0x1233, 0x1455, "iOptron", "iPolar"},
    {0x16c0, 0x29a0, "Orion", "Starshoot All-in-One"},
    {0, 0, NULL, NULL}
};
