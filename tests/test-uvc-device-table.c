#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../camlibs/uvc/uvc-devices.c"

int main(void)
{
    int ipolar_found = 0;
    int starshoot_found = 0;

    for (size_t i = 0; uvc_devices[i].vid != 0 || uvc_devices[i].pid != 0; ++i) {
        if (uvc_devices[i].vid == 0x1233 && uvc_devices[i].pid == 0x1455 &&
            strcmp(uvc_devices[i].vendor, "iOptron") == 0 &&
            strcmp(uvc_devices[i].model, "iPolar") == 0)
            ipolar_found++;
        if (uvc_devices[i].vid == 0x16c0 && uvc_devices[i].pid == 0x29a0)
            starshoot_found++;
    }

    if (ipolar_found != 1 || starshoot_found != 0) {
        fprintf(stderr, "UVC inventory mismatch: iPolar=%d StarShoot=%d\n",
                ipolar_found, starshoot_found);
        return 1;
    }
    puts("UVC identity table matches measured protocol classes");
    return 0;
}
