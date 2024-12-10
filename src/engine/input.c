#include "input.h"
#include "engine/window_manager.h"
#include <X11/X.h>
#include <stdint.h>
#include <engine/log.h>

#include <engine/input.h>

#include <editor/commands.h>

#include <engine/engine.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

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

void pe_input_init(){
    ZERO(input);
}

