//
// Created by pavon on 6/23/19.
//

#ifndef CAMERA_H
#define CAMERA_H 

#include <cglm/cglm.h>

#include "components/components.h"

void camera_rotate_control(float yaw, float pitch);
void camera_init(CameraComponent* camera);

void camera_update(CameraComponent* camera);

void camera_update_aspect_ratio(CameraComponent* camera);

CameraComponent main_camera;
CameraComponent saved_camera;

float camera_heigth_screen;
float camera_width_screen;
versor camera_rotation;

bool move_camera_input;

float camera_rotate_angle;

#endif //CAMERA_H
