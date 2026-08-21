//
// Created by pavon on 6/23/19.
//

#ifndef CAMERA_H
#define CAMERA_H 

#include <cglm/cglm.h>

typedef struct PCamera{
    mat4 projection;
    mat4 view;
    vec3 front;
    vec3 up;
    vec3 position;
}PCamera;

void camera_rotate_control(float yaw, float pitch);

void camera_init(PCamera* camera);

void camera_update(PCamera* camera);

void camera_update_aspect_ratio(PCamera* camera);

void camera_set_position(PCamera* camera, vec3 position);

void pe_camera_look_at(PCamera* camera, vec3 position);

extern float camera_height_screen;
extern float camera_width_screen;
extern versor camera_rotation;

extern bool move_camera_input;

extern float camera_rotate_angle;

extern PCamera saved_camera;
extern PCamera main_camera;

#endif //CAMERA_H
