#include <memory.h>
#include <stdio.h>

#include <Tactonic.h>
#include <TactonicTouch.h>

#include "device.h"

extern "C" {

struct device_s {
    int rows;
    int cols;

    int num_devices;

    TactonicDeviceList* device_list;

    TactonicDevice composite_device;
    TactonicDevice devices[MAX_DEVICES];

    TactonicFrame* composite_frame;
    TactonicFrame* frames[MAX_DEVICES];

    char update[MAX_DEVICES];
};

device_t* create_device() {
    return (device_t*)malloc(sizeof (device_t));
}

void destroy_device(device_t* device) {
    free(device);
}

int device_rows(const device_t* device) {
    return device->rows;
}

int device_cols(const device_t* device) {
    return device->cols;
}

void init_device_array(device_t* device) {
    device->device_list = Tactonic_GetDeviceList();
    device->num_devices = device->device_list->numDevices;
    if (device->num_devices < 1) {
        printf("No devices found.");
        exit(0);
    } else if (device->num_devices != 2 && device->num_devices != 4 &&
            device->num_devices != 7) {
        printf("%i devices connected; not a power of two", device->num_devices);
        exit(0);
    }

    int i;
    if (device->num_devices < 7) {
        for (i = 0; i < device->num_devices; i++) {
            device->devices[i] = device->device_list->devices[i];
            device->frames[i] = Tactonic_CreateFrame(device->devices[i]);
        }
    } else {
        for (i = 0; i < device->num_devices; i++) {
            // Sadly, this HAS to be hardcoded short of developing a complete
            // configuration app
            switch (device->device_list->devices[i].serialNumber) {
            case 18284640:
                device->devices[0] = device->device_list->devices[i];
                break;
            case 18284896:
                device->devices[1] = device->device_list->devices[i];
                break;
            case 18285408:
                device->devices[2] = device->device_list->devices[i];
                break;
            case 18285152:
                device->devices[3] = device->device_list->devices[i];
                break;
            case 18286432:
                device->devices[4] = device->device_list->devices[i];
                break;
            case 18286176:
                device->devices[5] = device->device_list->devices[i];
                break;
            case 18285920:
                device->devices[6] = device->device_list->devices[i];
                break;
//            case 18285664:
//               device->devices[7] = device->device_list->devices[i];
//               break;
            }
        }
        for (i = 0; i < device->num_devices; i++) {
            device->frames[i] = Tactonic_CreateFrame(device->devices[i]);
        }
    }

    device->composite_device = device->devices[0];
    if (device->num_devices == 2) {
        // Layout looks like:
        //   ___
        //  |_1_|
        //  |_2_|
        //
        device->composite_device.rows *= 2;
    } else if (device->num_devices == 4) {
        // Layout looks like:
        //   ___
        //  |_1_|
        //  |_2_|
        //  |_3_|
        //  |_4_|
        //
        device->composite_device.rows *= 4;
    } else if (device->num_devices == 7) {
        // Layout looks like:
        //   ___ ___
        //  |_1_|_5_|
        //  |_2_|_6_|
        //  |_3_|_7_|
        //  |_4_|_8_|
        //
        device->composite_device.rows *= 4;
        device->composite_device.cols *= 2;
    }
    for (i = 1; i < device->num_devices; i++) {
        device->composite_device.serialNumber += device->devices[i].serialNumber;
    }
    device->rows = device->composite_device.rows;
    device->cols = device->composite_device.cols;

    device->composite_frame = Tactonic_CreateFrame(device->composite_device);

    for (i = 0; i < device->num_devices; i++) {
        Tactonic_StartDevice(device->devices[i]);
        TactonicTouch_StartDetector(device->devices[i]);
    }
}

char poll_frame(device_t* device) {
    int i;
    for (i = 0; i < device->num_devices; i++) {
        double old_time = device->frames[i]->time;
        Tactonic_PollFrame(device->devices[i], device->frames[i]);

        if (device->frames[i]->time > old_time) {
            int num_forces = device->frames[i]->numForces;
            if (device->num_devices < 7) {
                int offset = num_forces * (device->num_devices - 1 - i);
                memcpy(&device->composite_frame->forces[offset],
                       device->frames[i]->forces,
                       sizeof (int) * num_forces);
            } else {
                // To make sense of the offsets, study the diagrams above.
                // It's important to know that forces are stored row-major,
                // and the rows are wider than the columns.
                int j;
                int rows = device->frames[i]->rows;
                int cols = device->frames[i]->cols;
                if (i < 4) {
                    // For the left side, we flip the rows horizontally
                    for (j = 0; j < rows; j++) {
                        int k;
                        int offset = (2 * i) * num_forces + 2 * j * cols;
                        for (k = 0; k < cols; k++) {
                            device->composite_frame->forces[offset + (cols - 1 - k)] =
                                device->frames[i]->forces[j * cols + k];
                        }
                    }
                } else {
                    // For the right side, we flip the columns vertically
                    for (j = 0; j < rows; j++) {
                        int offset = (2 * (i - 4)) * num_forces + (2 * j + 1) * cols;
                        memcpy(&device->composite_frame->forces[offset],
                               &device->frames[i]->forces[(rows - 1 - j) * cols],
                               sizeof (int) * cols);
                    }
                }
            }
            device->update[i] = 1;
        }
    }

    for (i = 0; i < device->num_devices; i++) {
        if (!device->update[i]) {
            return 0;
        }
    }

    device->composite_frame->time = device->frames[0]->time;

    for (i = 0; i < device->num_devices; i++) {
        device->update[i] = 0;
    }

    return 1;
}

void get_current_frame(const device_t* device, frame_t* frame) {
    frame->device = device;
    frame->time = device->composite_frame->time;
    frame->forces = device->composite_frame->forces;
    frame->num_forces = device->composite_frame->numForces;
    frame->rows = device->composite_frame->rows;
    frame->cols = device->composite_frame->cols;
}

}
