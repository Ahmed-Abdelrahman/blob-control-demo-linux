// TODO: Several objects are not freed if the program exits early (which it
// often does)
// TODO: Subroutines should not terminate the program.

#pragma once

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include <OVR_CAPI.h>

#include "controller.h"
#include "device.h"
#include "stick.h"

extern "C" {

const float RAD_TO_DEG = 180.0f / (2.0f * 3.14159265358979323f);

int main() {
    device_t* device = create_device();
    init_device_array(device);
    controller_info_t* controller = create_controller(device);
    stick_t* stick = acquire_stick();
    ovrSession session;
    ovrGraphicsLuid luid;
    {
        ovrResult result = ovr_Initialize(0);
        if (OVR_FAILURE(result)) {
            printf("Failed to initialize Rift SDK.");
            return 0;
        }
        result = ovr_Create(&session, &luid);
        if (OVR_FAILURE(result)) {
            printf("Failed to create Rift SDK session.");
            ovr_Shutdown();
            return 0;
        }
    }

    float c = 0.0f;
    float s = 0.0f;
    float radius = 0.0f;
    float direction[2] = { 0.0f, 0.0f };
    float yaw = 0.0f;
    while (1) {

        ovrTrackingState ts = ovr_GetTrackingState(
            session, ovr_GetTimeInSeconds(), 0);
        if (ts.StatusFlags & (ovrStatus_OrientationTracked |
                ovrStatus_PositionTracked)) {
            ovrPoseStatef pose = ts.HeadPose;
            ovrQuatf q = pose.ThePose.Orientation;
            yaw = atan2f(2 * (q.x * q.z + q.y * q.w),
                               1 - 2 * (q.y * q.y + q.z * q.z));
            c = cosf(yaw);
            s = sinf(yaw);
        }

        if (poll_frame(device)) {
            frame_t frame;
            get_current_frame(device, &frame);
            new_coordinates(controller, &frame, &radius, direction);
            print_debug(controller);
            printf("Rift.yaw: %f\xb0\n", yaw * RAD_TO_DEG);
            fflush(stdout);
        }

        stick_stat_t stat = {
            radius * (c * direction[0] + -s * direction[1]),
            radius * (s * direction[0] + c * direction[1])
        };

        write_stick(stick, &stat);
    }

    ovr_Destroy(session);
    ovr_Shutdown();
    destroy_stick(stick);
    destroy_controller(controller);
    destroy_device(device);
    return 0;
}

}
