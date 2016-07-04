#pragma once

extern "C" {

const size_t MAX_DEVICES = 8;

struct device_s;
typedef struct device_s device_t;

typedef struct frame_s {
    const device_t* device;
    double time;
    int* forces;
    int num_forces;
    int rows;
    int cols;
} frame_t;

device_t* create_device();
void destroy_device(device_t* device);

void init_device_array(device_t* device);

int device_rows(const device_t* device);
int device_cols(const device_t* device);
char poll_frame(device_t* device);
void get_current_frame(const device_t* device, frame_t* frame);

}
