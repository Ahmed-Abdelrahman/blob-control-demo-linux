#pragma once

#include <stdio.h>

#include "device.h"

const int DEVICE_WIDTH = 80;
const int DEVICE_HEIGHT = 96;

extern "C" {

int main() {
    device_t* device = create_device();
    frame_t frame;
    init_device_array(device);

    int img[DEVICE_HEIGHT][DEVICE_WIDTH];
    int row_factor = 1;
    int col_factor = 1;

    if (device_rows(device) > DEVICE_HEIGHT) {
        row_factor = 2;
    }
    if (device_cols(device) > DEVICE_WIDTH) {
        col_factor = 2;
    }

    while (1) {
        if (poll_frame(device)) {
            int i, j;

            printf("\034[2J");
            printf("\033[H");
            for (i = 0; i < DEVICE_HEIGHT; i++) {
                for (j = 0; j < DEVICE_WIDTH; j++) {
                    if (img[i][j] > 0) {
                        printf("o");
                    } else {
                        printf("/");
                    }
                    img[i][j] = 0;
                }
                printf("\n");
            }
            fflush(stdout);

            get_current_frame(device, &frame);
            for (i = 0; i < frame.rows; i++) {
                for (j = 0; j < frame.cols; j++) {
                    if (frame.forces[i * frame.cols + j] > 0) {
                        img[i / row_factor][j / col_factor] += 1;
                    }
                }
            }
        }
    }

    destroy_device(device);

    return 0;
}

}
