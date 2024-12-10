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
    input_key_size = sizeof(Input) / sizeof(Key);
    input.V.key_code = XKeysymToKeycode(display, XK_V);
}

void pe_parse_key_event(unsigned int key_code, uint8_t type){

    Key* this_input = (Key*)&input;


    for(uint8_t i = 0; i < input_key_size ; i++){
        Key* key = &this_input[i];
        if(key->key_code == key_code){
            if(type == KeyPress){
                key->pressed = true;
                LOG("found key\n");
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

  if (general_event.type == KeyPress) {
    XKeyPressedEvent *key_event = (XKeyPressedEvent *)&general_event;
    pe_parse_key_event(key_event->keycode, KeyPress);
  } else { // KeyRelease
    XKeyReleasedEvent *key_event = (XKeyReleasedEvent *)&general_event;
    pe_parse_key_event(key_event->keycode, KeyRelease);
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

#ifdef LINUX
void pe_input_character_callback(GLFWwindow* window, unsigned int codepoint){
    pe_input_character(codepoint);
}

void pe_input_mouse_movement_callback(GLFWwindow* window, double xpos, double ypos){
	if(mouse_navigate_control)
        mouse_movement_control(xpos, ypos);

    actual_mouse_position_x = xpos;
    actual_mouse_position_y = ypos;
         
}

void pe_input_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    Key* actual_key = NULL;
    Key* actual_mod_key = NULL;
    switch (key)
    {    
    case GLFW_KEY_LEFT_SHIFT:
        actual_mod_key = &input.SHIFT;
        break;
    case GLFW_KEY_LEFT_ALT:
        actual_mod_key = &input.ALT;
        break;
    case GLFW_KEY_BACKSPACE:
        actual_key = &input.BACKSPACE;
        break;
    case GLFW_KEY_ENTER:
        actual_key = &input.ENTER;
        break;
    case GLFW_KEY_A:
        actual_key = &input.A;
        break;
    case GLFW_KEY_B:
        actual_key = &input.B;
        break;
    case GLFW_KEY_C:
        actual_key = &input.C;
        break;
    case GLFW_KEY_D:
        actual_key = &input.D;
        break;
    case GLFW_KEY_E:
        actual_key = &input.E;
        break;
    case GLFW_KEY_F:
        actual_key = &input.F;
        break;
    case GLFW_KEY_G:
        actual_key = &input.G;
        break;
    case GLFW_KEY_H:
        actual_key = &input.H;
        break;
    case GLFW_KEY_I:
        actual_key = &input.I;
        break;
    case GLFW_KEY_J:
        actual_key = &input.J;
        break;
    case GLFW_KEY_K:
        actual_key = &input.K;
        break;
    case GLFW_KEY_L:
        actual_key = &input.L;
        break;
    case GLFW_KEY_M:
        actual_key = &input.M;
        break;
    case GLFW_KEY_N:
        actual_key = &input.N;
        break;
    case GLFW_KEY_O://letter "o"
        actual_key = &input.O;
        break;
    case GLFW_KEY_P:
        actual_key = &input.P;
        break;
    case GLFW_KEY_Q:
        actual_key = &input.Q;
        break;
    case GLFW_KEY_R:
        actual_key = &input.R;
        break;
    case GLFW_KEY_S:
        actual_key = &input.S;
        break;
    case GLFW_KEY_T:
        actual_key = &input.T;
        break;
    case GLFW_KEY_V:
        actual_key = &input.V;
        break;
    case GLFW_KEY_Y:
        actual_key = &input.Y;
        break;
    case GLFW_KEY_W:
        actual_key = &input.W;
        break;
    case GLFW_KEY_X:
        actual_key = &input.X;
        break;
    case GLFW_KEY_Z:
        actual_key = &input.Z;
        break;
    case GLFW_KEY_0://number zero
        actual_key = &input.KEY_0;
        break;
    case GLFW_KEY_1:
        actual_key = &input.KEY_1;
        break;    
    case GLFW_KEY_2:
        actual_key = &input.KEY_2;
        break;
    case GLFW_KEY_3:
        actual_key = &input.KEY_3;
        break;
    case GLFW_KEY_4:
        actual_key = &input.KEY_4;
        break;
    case GLFW_KEY_5:
        actual_key = &input.KEY_5;
        break;
    case GLFW_KEY_6:
        actual_key = &input.KEY_6;
        break;
    case GLFW_KEY_7:
        actual_key = &input.KEY_7;
        break;
    case GLFW_KEY_8:
        actual_key = &input.KEY_8;
        break;
    case GLFW_KEY_9:
        actual_key = &input.KEY_9;
        break;
    case GLFW_KEY_DOWN:
        actual_key = &input.KEY_DOWN;
        break;
    case GLFW_KEY_UP:
        actual_key = &input.KEY_UP;
        break;
    case GLFW_KEY_TAB:
        actual_key = &input.TAB;
        break;
    case GLFW_KEY_ESCAPE:
        actual_key = &input.ESC;
        break;
    
    default:
        break;
    }

    if(actual_key != NULL){
        if(action == GLFW_PRESS){
            actual_key->pressed = true;
            actual_key->Released = false;
            actual_key->mods = mods;
        }
        if(action == GLFW_RELEASE){
            actual_key->pressed = false;
            actual_key->Released = true;
            
        }
    } 
  
    if(actual_mod_key != NULL){
        if(action == GLFW_PRESS){
            actual_mod_key->pressed = true;
            actual_mod_key->Released = false;
        }
        if(action == GLFW_RELEASE){
            actual_mod_key->pressed = false;
            actual_mod_key->Released = true;
            
        }
    } 
}


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

#endif//define LINUX
void pe_input_init(){
    ZERO(input);
}

