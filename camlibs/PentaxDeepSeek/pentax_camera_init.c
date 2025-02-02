#include "pentax_enums.h"
#include <gphoto2/gphoto2.h>

void camera_init(GPhoto2Camera *camera) {
    // Initialize bulb mode functions
    camera->start_bulb_mode = start_bulb_mode;
    camera->stop_bulb_mode = stop_bulb_mode;

    // Initialize mirror-up functions
    camera->enable_mirror_up = enable_mirror_up;
    camera->disable_mirror_up = disable_mirror_up;
    camera->get_mirror_up_status = get_mirror_up_status;

    // Initialize file manipulation functions
    camera->create_directory = create_directory;
    camera->delete_file = delete_file;
    camera->copy_file = copy_file;
    camera->move_file = move_file;
}
