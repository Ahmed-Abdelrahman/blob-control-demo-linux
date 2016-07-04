// Code to acquire and control a virtual joystick

#pragma once

extern "C" {

struct stick_s;
typedef struct stick_s stick_t;

typedef struct stick_stat_s {
    // Axis values range from -1 to 1
    float axis_x;
    float axis_y;
} stick_stat_t;

// Attempts to take control of the first available vJoy device.
stick_t* acquire_stick();

void destroy_stick(stick_t* stick);

//void read_stick(const stick_t* stick, stick_stat_t* stat);
void write_stick(stick_t* stick, const stick_stat_t* stat);

}
