#include <stdio.h>

#include <windows.h>
#include <public.h>
#include <vjoyinterface.h>

#include "stick.h"

extern "C" {

const int DEFAULT_VJOY_DEV_ID = 1;
const int MAX_VJOY_DEV_ID = 16;

const long VJOY_AXIS_CENTER = 16000L;
const long VJOY_AXIS_RADIUS = 16000L;

struct stick_s {
    UINT device_id;
};

stick_t* acquire_stick() {
    stick_t* stick = (stick_t*)malloc(sizeof (stick_t));

    stick->device_id = 0;

    if (!vJoyEnabled()) {
        printf("Couldn't connect to vJoy. "
               "Make sure vJoy is installed and enabled");
        exit(0);
    } else {
        printf("vJoy enabled.\n");
        printf(" - Vendor: %s\n - Product: %s\n - Version: %s\n",
               (const char*)GetvJoyManufacturerString(),
               (const char*)GetvJoyProductString(),
               (const char*)GetvJoySerialNumberString()
               );
    }

    UINT i;
    for (i = DEFAULT_VJOY_DEV_ID; i <= MAX_VJOY_DEV_ID; i++) {
        VjdStat status = GetVJDStatus(i);
        if (status == VJD_STAT_OWN || status == VJD_STAT_FREE) {
            stick->device_id = i;
            break;
        }
    }
    if (stick->device_id == 0) {
        printf("No available vJoy devices.");
        exit(0);
    }

    if (!AcquireVJD(stick->device_id)) {
        printf("Failed to acquire vJoy device %i", stick->device_id);
        RelinquishVJD(stick->device_id);
        exit(0);
    } else {
        printf("Acquired vJoy device %i", stick->device_id);
    }

    return stick;
}

void destroy_stick(stick_t* stick) {
    RelinquishVJD(stick->device_id);
    free(stick);
}

void write_stick(stick_t* stick, const stick_stat_t* stat) {
    BYTE id = (BYTE)stick->device_id;
    JOYSTICK_POSITION_V2 report;
    report.bDevice = id;
    report.wAxisX = VJOY_AXIS_CENTER + (long)(stat->axis_x * VJOY_AXIS_RADIUS);
    report.wAxisY = VJOY_AXIS_CENTER + (long)(stat->axis_y * VJOY_AXIS_RADIUS);
    report.wAxisXRot = VJOY_AXIS_CENTER;
    report.wAxisYRot = VJOY_AXIS_CENTER;
    if (!UpdateVJD(stick->device_id, &report)) {
        printf("Can't write to vJoy device %i. "
               "Try to free or re-enable the device and press any character "
               "to continue.",
               stick->device_id);
        getchar();
        AcquireVJD(stick->device_id);
    }
}

}
