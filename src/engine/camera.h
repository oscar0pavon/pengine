//
// Created by pavon on 6/23/19.
//

#ifndef CAMERA_H
#define CAMERA_H 

#include <cglm/cglm.h>
#include "engine/components/components.h"
#include "renderer/vulkan.h"

#define WINDOW_HEIGHT 720
#define WINDOW_WIDTH 1280

void camera_rotate_control(float yaw, float pitch);
void camera_init(Camera* camera);

void camera_update(Camera* camera);

void camera_update_aspect_ratio(Camera* camera);

void camera_set_position(Camera* camera, vec3 position);

extern float camera_height_screen;
extern float camera_width_screen;
extern versor camera_rotation;

extern bool move_camera_input;

extern float camera_rotate_angle;

void pe_camera_look_at(Camera* camera, vec3 position);

CameraComponent saved_camera;
CameraComponent main_camera;

#endif //CAMERA_H
