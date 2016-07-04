#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "controller.h"
#include "device.h"
#include "signals.h"

extern "C" {

// All of these are made up
const float DEAD_ZONE_RADIUS = 0.33f;

const float MIN_FORCE = 20000;

// A smoothing and differentiating kernel
const float DIFF_KERNEL[4] = {
    0.7f, -0.5f, -0.1f, -0.1f
};

const float STEP_HYSTERESIS_HIGH = 25000;
const float STEP_HYSTERESIS_LOW = 10000;

const float FREQ_LEVELS[5] = {
    0.05f, 0.1f, 0.15f, 0.2f, 0.25f
};

const int FREQUENCY_FRAMES[5] = {
    7, 18, 16, 14, 13
};

const float TRANSITION_KERNEL[4] = {
    0.165f, 0.33f, 0.33f, 0.165f
};

// Determines whether average direction ill-defined
const float MIN_DIR_LEN_SQ = 1.0e-4f;

const int DIR_FRAMES[5] = {
    5, 14, 12, 10, 9
};

struct controller_info_s {
    device_t* device;

    float center[2];

    float* force_coeffs;
    float* dir_coeffs[2];

    cyclic_buf_t sig_force;
    cyclic_buf_t sig_steps;
    int hysteresis_level;
    // Number of frames to average when determining frequency
    int frequency_frames;
    cyclic_buf_t sig_frequency;
    cyclic_buf_t sig_radius;

    cyclic_buf2_t sig_dir;
    // Average from this many recent frames
    // Can be set to 0 for sudden turning
    int dir_frames_to_average;
    cyclic_buf2_t sig_final_dir;
};

controller_info_t* create_controller(device_t* device) {
    controller_info_t* controller =
        (controller_info_t*)malloc(sizeof(controller_info_t));
    controller->device = device;
    controller->sig_force = EMPTY_BUF;
    controller->sig_steps = EMPTY_BUF;
    controller->sig_frequency = EMPTY_BUF;
    controller->sig_radius = EMPTY_BUF;
    controller->sig_dir = EMPTY_BUF2;
    controller->sig_final_dir = EMPTY_BUF2;
    controller->hysteresis_level = 0;
    controller->frequency_frames = 1;
    controller->dir_frames_to_average = 1;

    int rows = device_rows(device);
    int cols = device_cols(device);
    controller->center[0] = (rows - 1) / 2.0f;
    controller->center[1] = (cols - 1) / 2.0f;

    controller->force_coeffs = (float*)malloc(rows * cols * sizeof(float));
    controller->dir_coeffs[0] = (float*)malloc(rows * cols * sizeof(float));
    controller->dir_coeffs[1] = (float*)malloc(rows * cols * sizeof(float));

    float r2;
    if (rows < cols) {
        float r = DEAD_ZONE_RADIUS * rows / 2;
        r2 = r * r;
    }
    else {
        float r = DEAD_ZONE_RADIUS * cols / 2;
        r2 = r * r;
    }
    int i, j;
    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            float x = (i - controller->center[0]);
            float y = (j - controller->center[1]);
            float lensq = x * x + y * y;
            if (lensq < r2) {
                controller->force_coeffs[i * cols + j] = 0.0f;
                controller->dir_coeffs[0][i * cols + j] = 0.0f;
                controller->dir_coeffs[1][i * cols + j] = 0.0f;
            }
            else {
                float len = sqrtf((float)lensq);
                controller->force_coeffs[i * cols + j] = 1.0f;
                controller->dir_coeffs[0][i * cols + j] = x / len;
                controller->dir_coeffs[1][i * cols + j] = y / len;
            }
        }
    }

    return controller;
}

void destroy_controller(controller_info_t* controller) {
    free(controller->force_coeffs);
    free(controller->dir_coeffs[0]);
    free(controller->dir_coeffs[1]);
    free(controller);
}

void print_debug(controller_info_t* controller) {
    float* avg_dir = cyclic_index2(&controller->sig_dir, 0);
    float* final_dir = cyclic_index2(&controller->sig_final_dir, 0);

    printf("\033[2J");
    printf("\033[H");
    printf("Force: %f\nDir: (%f, %f)\tAngle: %f rad\n",
        cyclic_index(&controller->sig_force, 0),
        avg_dir[0], avg_dir[1],
        atan2(avg_dir[1], avg_dir[0]));
    if (controller->hysteresis_level == -1) {
        printf("Stepping: Up\n");
    } else if (controller->hysteresis_level == 1) {
        printf("Stepping: Down\n");
    } else {
        printf("Stepping: -\n");
    }
    printf("Frequency: %f\n", cyclic_index(&controller->sig_frequency, 0));
    printf("Radius: %f\n", cyclic_index(&controller->sig_radius, 0));
    printf("Final dir: (%f, %f)\n", final_dir[0], final_dir[1]);
    fflush(stdout);
}

void new_coordinates(controller_info_t* controller,
    const frame_t* frame,
    float* radius,
    float* direction) {

    // Compute force and direction

    float total_force = 0.0f;
    float avg_dir[2] = { 0.0f, 0.0f };
    int i, j;
    for (i = 0; i < frame->rows; i++) {
        for (j = 0; j < frame->cols; j++) {
            // Is it faster to check if outside dead zone here?
            int idx = i * frame->cols + j;
            total_force += controller->force_coeffs[idx] * frame->forces[idx];
            avg_dir[0] += controller->dir_coeffs[0][idx] * frame->forces[idx];
            avg_dir[1] += controller->dir_coeffs[1][idx] * frame->forces[idx];
        }
    }

    float dir_len_sq = avg_dir[0] * avg_dir[0] + avg_dir[1] * avg_dir[1];
    if (dir_len_sq < MIN_DIR_LEN_SQ || total_force < MIN_FORCE) {
        avg_dir[0] = 0.0f;
        avg_dir[1] = 0.0f;
    } else {
        float dir_len = sqrtf(dir_len_sq);
        avg_dir[0] /= dir_len;
        avg_dir[1] /= dir_len;
    }

    cyclic_insert(&controller->sig_force, total_force);
    cyclic_insert2(&controller->sig_dir, avg_dir);

    // Identify steps

    float diff = convolve_point(&controller->sig_force, 0, DIFF_KERNEL, 4);
    switch (controller->hysteresis_level) {
    case -1:
        if (diff > STEP_HYSTERESIS_HIGH) {
            controller->hysteresis_level = 1;
            cyclic_insert(&controller->sig_steps, 1);
        } else {
            if (diff > -STEP_HYSTERESIS_LOW) {
                controller->hysteresis_level = 0;
            }
            cyclic_insert(&controller->sig_steps, 0);
        }
    case 0:
        if (diff > STEP_HYSTERESIS_HIGH) {
            controller->hysteresis_level = 1;
            cyclic_insert(&controller->sig_steps, 1);
        } else if (diff < -STEP_HYSTERESIS_LOW) {
            controller->hysteresis_level = -1;
            cyclic_insert(&controller->sig_steps, -1);
        } else {
            cyclic_insert(&controller->sig_steps, 0);
        }
    case 1:
        if (diff < -STEP_HYSTERESIS_HIGH) {
            controller->hysteresis_level = -1;
            cyclic_insert(&controller->sig_steps, -1);
        } else {
            if (diff < STEP_HYSTERESIS_LOW) {
                controller->hysteresis_level = 0;
            }
            cyclic_insert(&controller->sig_steps, 0);
        }
    }

    // Update counters

    float* prev_dir = cyclic_index2(&controller->sig_final_dir, 1);
    if (prev_dir[0] * avg_dir[0] + prev_dir[1] * avg_dir[1] < -0.25f) {
        controller->frequency_frames = 1;
        controller->dir_frames_to_average = 1;
    }

    int level;

    if (cyclic_index(&controller->sig_frequency, 0) < FREQ_LEVELS[0]) {
        level = 0;
    } else if (cyclic_index(&controller->sig_frequency, 0) < FREQ_LEVELS[1]) {
        level = 1;
    } else if (cyclic_index(&controller->sig_frequency, 0) < FREQ_LEVELS[2]) {
        level = 2;
    } else if (cyclic_index(&controller->sig_frequency, 0) < FREQ_LEVELS[3]) {
        level = 3;
    } else {
        level = 4;
    }

    if (controller->frequency_frames < FREQUENCY_FRAMES[level]) {
        controller->frequency_frames++;
    } else if (controller->frequency_frames > FREQUENCY_FRAMES[level]) {
        controller->frequency_frames--;
    }
    if (controller->dir_frames_to_average < DIR_FRAMES[level]) {
        controller->dir_frames_to_average++;
    } else if (controller->dir_frames_to_average > DIR_FRAMES[level]) {
        controller->dir_frames_to_average--;
    }

    // Update frequency

    float frequency = 0;
    if (cyclic_index(&controller->sig_frequency, 0) < FREQ_LEVELS[0]) {
        // When moving very slowly or standing still, ignore up steps
        for (i = 0; i < controller->frequency_frames; i++) {
            float a = cyclic_index(&controller->sig_steps, i);
            if (a > 0) {
                frequency += a;
            }
        }
    } else {
        for (i = 0; i < controller->frequency_frames; i++) {
            frequency += fabsf(cyclic_index(&controller->sig_steps, i));
        }
    }
    frequency /= controller->frequency_frames;
    cyclic_insert(&controller->sig_frequency, frequency);

    // Set radius

    float final_radius = 0.0f;
    float transition_level = convolve_point(&controller->sig_frequency, 0,
        TRANSITION_KERNEL, 4);
    if (transition_level > 0.25f) {
        final_radius = 1.0f;
    } else {
        // This constant too
        final_radius = 4.0f * transition_level;
    }
    cyclic_insert(&controller->sig_radius, final_radius);

    // Average out direction

    float final_dir[2] = { 0.0f, 0.0f };
    for (i = 0; i < controller->dir_frames_to_average; i++) {
        prev_dir = cyclic_index2(&controller->sig_dir, i);
        final_dir[0] += prev_dir[0];
        final_dir[1] += prev_dir[1];
    }
    float final_len_sq = final_dir[0] * final_dir[0] + final_dir[1] * final_dir[1];
    if (final_len_sq < MIN_DIR_LEN_SQ) {
        final_dir[0] = 0.0f;
        final_dir[1] = 0.0f;
    }  else {
        float final_len = sqrtf(final_len_sq);
        final_dir[0] /= final_len;
        final_dir[1] /= final_len;
    }
    cyclic_insert2(&controller->sig_final_dir, final_dir);

    *radius = final_radius;
    direction[0] = final_dir[0];
    direction[1] = final_dir[1];
}

}
