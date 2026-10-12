//
// Created by pavon on 6/23/19.
//

#include "input.h"
#include <android/keycodes.h>
#include <pengine/input.h>
#include <pengine/log.h>
#include <pengine/engine.h>

static inline void touch_position(float x, float y) {
  LOG("------> X: %f , Y: %f", touch_position_x, touch_position_y);
}

void handle_input_event(struct AInputEvent *event) {
  touch_position_x = AMotionEvent_getX(event, 0);
  touch_position_y = AMotionEvent_getY(event, 0);
}

void handle_finger_up() {
  touch_position_x = -1;
  touch_position_y = -1;
}

int handle_input(android_app *app, AInputEvent *event) {

  int32_t eventType = AInputEvent_getType(event);
  switch (eventType) {
  case AINPUT_EVENT_TYPE_MOTION:
    switch (AInputEvent_getSource(event)) {
    case AINPUT_SOURCE_TOUCHSCREEN: {

      int action = AKeyEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
      switch (action) {
      case AMOTION_EVENT_ACTION_DOWN:
        handle_input_event(event);
        break;
      case AMOTION_EVENT_ACTION_UP:
        handle_finger_up();

        break;
      case AMOTION_EVENT_ACTION_MOVE:
        handle_input_event(event);
        break;
      }
    } break;
    } // end switch
    break;
  case AINPUT_EVENT_TYPE_KEY:
    // handle key input...
    break;
  } // end switch
  return -1;
}

void pe_android_input_single_key(int32_t action, int32_t code) {

  Key *current_key = NULL;

  switch (code) {
  case AKEYCODE_ESCAPE:
    current_key = &input.ESC;
    break;
  case AKEYCODE_A:
    current_key = &input.A;
    break;
  case AKEYCODE_B:
    current_key = &input.B;
    break;
  case AKEYCODE_C:
    current_key = &input.C;
    break;
  case AKEYCODE_D:
    current_key = &input.D;
    break;
  case AKEYCODE_E:
    current_key = &input.E;
    break;
  case AKEYCODE_F:
    current_key = &input.F;
    break;
  case AKEYCODE_G:
    current_key = &input.G;
    break;
  case AKEYCODE_H:
    current_key = &input.H;
    break;
  case AKEYCODE_I:
    current_key = &input.I;
    break;
  case AKEYCODE_J:
    current_key = &input.J;
    break;
  case AKEYCODE_K:
    current_key = &input.K;
    break;
  case AKEYCODE_L:
    current_key = &input.L;
    break;
  case AKEYCODE_M:
    current_key = &input.M;
    break;
  case AKEYCODE_N:
    current_key = &input.N;
    break;
  case AKEYCODE_O:
    current_key = &input.O;
    break;
  case AKEYCODE_P:
    current_key = &input.P;
    break;
  case AKEYCODE_Q:
    current_key = &input.Q;
    break;
  case AKEYCODE_R:
    current_key = &input.R;
    break;
  case AKEYCODE_S:
    current_key = &input.S;
    break;
  case AKEYCODE_T:
    current_key = &input.T;
    break;
  case AKEYCODE_U:
    current_key = &input.U;
    break;
  case AKEYCODE_V:
    current_key = &input.V;
    break;
  case AKEYCODE_W:
    current_key = &input.W;
    break;
  case AKEYCODE_X:
    current_key = &input.X;
    break;
  case AKEYCODE_Y:
    current_key = &input.Y;
    break;
  case AKEYCODE_Z:
    current_key = &input.Z;
    break;
  case AKEYCODE_TAB:
    current_key = &input.TAB;
    break;
  case AKEYCODE_ENTER:
    current_key = &input.ENTER;
    break;
  case AKEYCODE_SEMICOLON:
    current_key = &input.SEMICOLON;
    break;
  default:
    break;
  }

  if (current_key) {
    if (action == AKEY_EVENT_ACTION_DOWN) {
      current_key->pressed = true;
      current_key->Released = false;
    }
    if (action == AKEY_EVENT_ACTION_UP) {
      current_key->pressed = false;
      current_key->Released = true;
    }
  }
}

int32_t pe_android_input_handle(struct android_app *app, AInputEvent *event) {
  if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {
    int32_t action = AKeyEvent_getAction(event);
    int32_t key_code = AKeyEvent_getKeyCode(event);
    pe_android_input_single_key(action, key_code);//for standard input AWSD 
    
    return 1;
  }
  


  if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_DRAG) {

    //		engine->touchX = AMotionEvent_getX(event, 0);
    //		engine->touchY = AMotionEvent_getY(event, 0);
    //		ALOGI("x %d\ty %d\n",engine->touchX,engine->touchY);
    return 1;
  }

  float touch_x = AMotionEvent_getX(event, 0);
  float touch_y = AMotionEvent_getY(event, 0);

  //    LOG("##### Input touch X: %f Y: %f", touch_x, touch_y);
  touch_position_x = touch_x;
  touch_position_y = touch_y;

  //  LOG("################ Input handle ####################");
  return 0;
}
