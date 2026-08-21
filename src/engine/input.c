#include "input.h"
#include "engine/window_manager.h"
#include <linux/input-event-codes.h>
#include <stdint.h>
#include <engine/log.h>

#include <engine/input.h>

#include <engine/engine.h>

uint8_t input_key_size;

//INFO one evdev code per Input member, in the order the members are declared
//in input.h - pe_input_init() walks the struct as a Key array, the same way
//pe_parse_key_event() does. the struct is not alphabetical, so read it rather
//than assuming. the static assert below fails if a member is added here or
//there without the other
static const unsigned char pe_key_codes[] = {
    KEY_A,     KEY_B,     KEY_C,     KEY_D,         KEY_E,     KEY_F,
    KEY_G,     KEY_H,     KEY_I,     KEY_J,         KEY_K,     KEY_L,
    KEY_M,     KEY_N,     KEY_O,     KEY_P,         KEY_Q,     KEY_R,
    KEY_S,     KEY_T,     KEY_X,     KEY_U,         KEY_V,     KEY_Y,
    KEY_Z,     KEY_W,     KEY_TAB,   KEY_SPACE,     KEY_ESC,   KEY_LEFTSHIFT,
    KEY_ENTER, KEY_0,     KEY_1,     KEY_2,         KEY_3,     KEY_4,
    KEY_5,     KEY_6,     KEY_7,     KEY_8,         KEY_9,     KEY_BACKSPACE,
    KEY_SEMICOLON, KEY_LEFTALT, KEY_UP, KEY_DOWN};

_Static_assert(sizeof(pe_key_codes) / sizeof(pe_key_codes[0]) ==
                   sizeof(Input) / sizeof(Key),
               "pe_key_codes needs one entry per Input member, in order");

void pe_parse_key_event(unsigned int key_code, uint8_t type){

    Key* this_input = (Key*)&input;

    for(uint8_t i = 0; i < input_key_size ; i++){
        Key* key = &this_input[i];
        if(key->key_code == key_code){
            if(type == PE_KEY_PRESSED){
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

//INFO the codes are raw evdev, which is what a wayland compositor hands to
//pway and pway hands to pe_parse_key_event(). the X11 keymap this replaced
//had to ask the server for a keycode per key, so it could only be built
//after a window existed
void pe_input_init(){
    ZERO(input);
    input_key_size = sizeof(Input) / sizeof(Key);

    Key* keys = (Key*)&input;

    for(uint8_t i = 0; i < input_key_size; i++)
        keys[i].key_code = pe_key_codes[i];
}

void pe_input_clean(){

    Key* this_input = (Key*)&input;

    for(uint8_t i = 0; i < input_key_size ; i++){
        Key* key = &this_input[i];
        key->Released = false;
        key->pressed = false;
    }
}

