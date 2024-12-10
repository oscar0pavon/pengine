#include "input.h"
#include "camera.h"
#include "engine/window_manager.h"
#include <X11/X.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <engine/log.h>

#include <engine/input.h>

#include <editor/commands.h>

#include <engine/engine.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

bool left_click = false;
float actual_mouse_position_x;
float actual_mouse_position_y;
uint8_t input_key_size;

void pe_init_x11_keys(){
    XSelectInput(display, window, KeyPressMask | KeyReleaseMask );
    input_key_size = sizeof(Input) / sizeof(Key);
    input.V.key_code = XKeysymToKeycode(display, XK_V);
    input.Q.key_code = XKeysymToKeycode(display, XK_Q);
}

void pe_parse_key_event(unsigned int key_code, uint8_t type){

    Key* this_input = (Key*)&input;

    for(uint8_t i = 0; i < input_key_size ; i++){
        Key* key = &this_input[i];
        if(key->key_code == key_code){
            if(type == KeyPress){
                key->pressed = true;
                return;
            }else{//Released
                key->Released = true;
                key->pressed = false;
                return;
            }
        }
    }

    LOG("not key code\n");

}

void pe_wm_poll_events_x11() {
  XEvent general_event = {};
  XNextEvent(display, &general_event);

  if (general_event.type == KeyRelease) { // KeyRelease
    XKeyReleasedEvent *key_event = (XKeyReleasedEvent *)&general_event;
    pe_parse_key_event(key_event->keycode, KeyRelease);
  }
  if (general_event.type == KeyPress) {
    XKeyPressedEvent *key_event = (XKeyPressedEvent *)&general_event;
    pe_parse_key_event(key_event->keycode, KeyPress);
  } 
}

void mouse_movement_control(float xpos, float ypos){   

    horizontalAngle += camera_width_screen/2 - xpos ;
    
    verticalAngle  += camera_heigth_screen/2 - ypos ;

    horizontalAngle *= 0.05;
    verticalAngle *= 0.05;

    camera_rotate_control(0, horizontalAngle);   
     
}

int char_pressed = 0;


void pe_input_mouse_button_callback(GLFWwindow* window, int button, int action, int mods){

  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {

      move_camera_input = false;
      mouse_navigate_control = false;
      // change_to_editor_mode(EDITOR_DEFAULT_MODE);
      left_click = true;
      touch_position_x = actual_mouse_position_x;
      touch_position_y = actual_mouse_position_y;
    }
    if (action == GLFW_RELEASE) {
      left_click = false;
      touch_position_x = -1;
      touch_position_y = -1;
    }
  }
  if (button == GLFW_MOUSE_BUTTON_RIGHT) {
    if (action == GLFW_PRESS) {
      move_camera_input = true;
      mouse_navigate_control = true;
      // change_to_editor_mode(EDITOR_NAVIGATE_MODE);
    }
    if (action == GLFW_RELEASE) {
    }
  }
}

void pe_input_init(){
    ZERO(input);
}

