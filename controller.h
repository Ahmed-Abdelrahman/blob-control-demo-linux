#pragma once

// Here we find all the signal processing logic that determines the
// direction and degree of tilt of the virtual joystick. The cursor
// files provide interpolation between frames.

extern "C" {

struct controller_info_s;
typedef struct controller_info_s controller_info_t;

struct device_s;
typedef struct device_s device_t;
struct frame_s;
typedef struct frame_s frame_t;

// Create/initialize and destroy
controller_info_t* create_controller(device_t* device);
void destroy_controller(controller_info_t* controller);

void print_debug(controller_info_t* controller);

void new_coordinates(controller_info_t* controller,
                     const frame_t* frame,
                     float* radius,
                     float* direction);

}
