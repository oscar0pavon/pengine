//

#include "camera.h"
#include <cglm/cam.h>
#include <cglm/types.h>
#include "renderer/renderer.h"
#include "utils.h"
#include "window_manager.h"
//#include "window.h"
//INFO glm_perspective() takes the angle in radians - it goes straight into
//tanf() - so the 45 that was passed here was 45 radians, which wrapped round
//to a vertical field of view of about 58 degrees. near enough to look right
//and never to be questioned, but not the angle anyone asked for
#define PE_CAMERA_FOV_DEGREES 45.f

//INFO the near plane is what decides how much of the depth buffer is left for
//everything else: at 0.001 with this far plane, a 5000000 to 1 range, the
//whole of a terrain tile past a hundred yards landed in a few hundred of the
//float's values. adjacent triangles then came out at the same depth or over
//the far plane, and the ground tore open along thin cracks that moved with
//the camera. nothing here needs to draw closer than this
#define PE_CAMERA_NEAR 0.1f
#define PE_CAMERA_FAR 5000.f

bool first_camera_rotate = true;

vec3 init_front;

float camera_height_screen;
float camera_width_screen;
versor camera_rotation;

bool move_camera_input;

float camera_rotate_angle;

PCamera saved_camera;
PCamera main_camera;

void camera_init(PCamera* camera){
    camera_init_with_size(camera, pe_window_width, pe_window_height);
}

void camera_init_with_size(PCamera* camera, float width, float height){
    camera_width_screen = width;
    camera_height_screen = height;

    camera_rotate_angle = 0;

    glm_mat4_identity(camera->view);
    glm_mat4_identity(camera->projection);

    init_vec3(1.0f, 0 ,  0.0f, camera->front);
    init_vec3(0.0f, 0.0f,  1.0f, camera->up);
    init_vec3(0,0,0, camera->position);

    vec3 look_pos;
    glm_vec3_add(camera->position, camera->front, look_pos);

    glm_lookat(camera->position, look_pos, camera->front , camera->view);

    glm_perspective(glm_rad(PE_CAMERA_FOV_DEGREES), width / height,
                    PE_CAMERA_NEAR, PE_CAMERA_FAR, camera->projection);


    //INFO vulkan's clip space has +Y pointing down where GL has it pointing up
    camera->projection[1][1] *= -1;
}

void camera_set_position(PCamera* camera, vec3 position){

  init_vec3(position[0], position[1], position[2], camera->position);
  camera_update(camera);

}

void camera_update(PCamera* camera){
    vec3 look_pos;
    glm_vec3_add(camera->position, camera->front, look_pos);

    glm_lookat(camera->position, look_pos, camera->up , camera->view);
}

void pe_camera_look_at(PCamera* camera, vec3 position){
    glm_lookat(camera->position, position, camera->up, camera->view); 
}

void camera_rotate_control(float yaw, float pitch){
    vec3 front;

    front[0] = cos(glm_rad(yaw)) * cos(glm_rad(pitch));
    front[1] = sin(glm_rad(pitch));
    front[2] = sin(glm_rad(yaw)) * cos(glm_rad(pitch));

    if(first_camera_rotate == true){
        glm_vec3_copy(main_camera.front,init_front);
        glm_vec3_mul(init_front,(vec3){0,5,0},init_front);
        first_camera_rotate = false;
    }
 
    glm_normalize(front);

    glm_vec3_copy(front, main_camera.front);

}

void camera_update_aspect_ratio(PCamera* camera){
  glm_perspective(glm_rad(PE_CAMERA_FOV_DEGREES),
                  camera_width_screen / camera_height_screen, PE_CAMERA_NEAR,
                  PE_CAMERA_FAR, camera->projection);

  //INFO the same flip camera_init_with_size() ends on: vulkan's clip space has
  //+Y pointing down where GL has it pointing up. without it here a camera that
  //had been through a resize drew the whole scene upside down
  camera->projection[1][1] *= -1;
}
