//
// Created by pavon on 6/23/19.
//

#ifndef PAVONTHEGAME_INPUT_H
#define PAVONTHEGAME_INPUT_H

#include <engine/utils.h>
typedef struct android_app android_app;

#include <android/input.h>
int handle_input(android_app* app, AInputEvent* event);
int32_t pe_android_input_handle(struct android_app *app, AInputEvent *event);


#endif //PAVONTHEGAME_INPUT_H
