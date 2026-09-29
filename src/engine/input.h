#ifndef PE_INPUT_H
#define PE_INPUT_H

#ifdef ANDROID
#include <engine/platforms/android/input.h>
#endif

#include <stdbool.h>
#include <stdint.h>

//INFO the key state pe_parse_key_event() takes. the values are
//WL_KEYBOARD_KEY_STATE_RELEASED/PRESSED, so a pway key callback can forward
//its state straight through
typedef enum PEKeyState {
	PE_KEY_RELEASED = 0,
	PE_KEY_PRESSED = 1
} PEKeyState;

typedef struct Key{
	bool pressed;
	bool Released;
	unsigned char key_code;
	int mods;
}Key;

typedef struct Input {
	struct Key A;
	struct Key B;
	struct Key C;
	struct Key D;
	struct Key E;
	struct Key F;
	struct Key G;
	struct Key H;
	struct Key I;
	struct Key J;
	struct Key K;
	struct Key L;
	struct Key M;
	struct Key N;
	struct Key O;
	struct Key P;
	struct Key Q;
	struct Key R;
	struct Key S;
	struct Key T;
	struct Key X;
	struct Key U;
	struct Key V;
	struct Key Y;
	struct Key Z;
	struct Key W;
	struct Key TAB;
	struct Key SPACE;
	struct Key ESC;
	struct Key SHIFT;
	struct Key ENTER;
	struct Key KEY_0;
	struct Key KEY_1;
	struct Key KEY_2;
	struct Key KEY_3;
	struct Key KEY_4;
	struct Key KEY_5;
	struct Key KEY_6;
	struct Key KEY_7;
	struct Key KEY_8;
	struct Key KEY_9;
	struct Key BACKSPACE;
	struct Key SEMICOLON;
	struct Key ALT;
	struct Key KEY_UP;
	struct Key KEY_DOWN;
}Input;


struct Input input;

static inline bool key_released(Key* key){
    if(key->Released){
        key->Released = false;
        return true;
    }
    return false;
}

static inline bool key__released(Key* key, int mods){
	if(key->Released){
		if(key->mods == mods){
			key->Released = false;
        	return true;
		}        
    }
    return false;
}

float touch_position_x;
float touch_position_y;


float horizontalAngle;
float verticalAngle;

bool mouse_navigate_control;

//desktop mouse: surface-local position, updated on every wl_pointer motion
//event, and the three buttons, which follow the same pressed/Released
//contract as a keyboard Key (key_released() works on them unchanged). No
//delta here - a consumer that wants a look delta keeps its own last-position
//static and subtracts, same as it would with any other polled position
typedef struct Mouse {
	float x;
	float y;
	Key left;
	Key right;
	Key middle;
} Mouse;

struct Mouse mouse;

void pe_input_init();

void pe_parse_key_event(unsigned int key_code, uint8_t type);

void pe_parse_mouse_motion(float x, float y);
void pe_parse_mouse_button(uint32_t button_id, bool pressed);


void mouse_movement_control(float xpos, float ypos);


void pe_input_clean();

#endif
