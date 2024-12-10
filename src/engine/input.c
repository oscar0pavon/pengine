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

    input.A.key_code = XKeysymToKeycode(display, XK_A);
    input.B.key_code = XKeysymToKeycode(display, XK_B);
    input.C.key_code = XKeysymToKeycode(display, XK_C);
    input.D.key_code = XKeysymToKeycode(display, XK_D);
    input.E.key_code = XKeysymToKeycode(display, XK_E);
    input.F.key_code = XKeysymToKeycode(display, XK_F);
    input.G.key_code = XKeysymToKeycode(display, XK_G);
    input.H.key_code = XKeysymToKeycode(display, XK_H);
    input.I.key_code = XKeysymToKeycode(display, XK_I);
    input.J.key_code = XKeysymToKeycode(display, XK_J);
    input.K.key_code = XKeysymToKeycode(display, XK_K);
    input.L.key_code = XKeysymToKeycode(display, XK_L);
    input.M.key_code = XKeysymToKeycode(display, XK_M);
    input.N.key_code = XKeysymToKeycode(display, XK_N);
    input.O.key_code = XKeysymToKeycode(display, XK_O);
    input.P.key_code = XKeysymToKeycode(display, XK_P);
    input.Q.key_code = XKeysymToKeycode(display, XK_Q);
    input.R.key_code = XKeysymToKeycode(display, XK_R);
    input.S.key_code = XKeysymToKeycode(display, XK_S);
    input.T.key_code = XKeysymToKeycode(display, XK_T);
    input.U.key_code = XKeysymToKeycode(display, XK_U);
    input.V.key_code = XKeysymToKeycode(display, XK_V);
    input.W.key_code = XKeysymToKeycode(display, XK_W);
    input.X.key_code = XKeysymToKeycode(display, XK_X);
    input.Y.key_code = XKeysymToKeycode(display, XK_Y);
    input.Z.key_code = XKeysymToKeycode(display, XK_Z);

    input.KEY_0.key_code = XKeysymToKeycode(display, XK_0);
    input.KEY_1.key_code = XKeysymToKeycode(display, XK_1);
    input.KEY_2.key_code = XKeysymToKeycode(display, XK_2);
    input.KEY_3.key_code = XKeysymToKeycode(display, XK_3);
    input.KEY_4.key_code = XKeysymToKeycode(display, XK_4);
    input.KEY_5.key_code = XKeysymToKeycode(display, XK_5);
    input.KEY_6.key_code = XKeysymToKeycode(display, XK_6);
    input.KEY_7.key_code = XKeysymToKeycode(display, XK_7);
    input.KEY_8.key_code = XKeysymToKeycode(display, XK_8);
    input.KEY_9.key_code = XKeysymToKeycode(display, XK_9);
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

void pe_input_clean(){

    Key* this_input = (Key*)&input;

    for(uint8_t i = 0; i < input_key_size ; i++){
        Key* key = &this_input[i];
        key->Released = false;
        key->pressed = false;
    }
}

