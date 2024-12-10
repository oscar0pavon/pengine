//
// Created by pavon on 6/24/19.
//

#ifndef PAVON_MOBILE_GAME_H
#define PAVON_MOBILE_GAME_H

#include "model.h"

#include <engine/window_manager.h>

typedef struct PGame{
	char* name;
	void(*init)(void);
	void(*loop)(void);
	void(*end)(void);
	void(*input)(void);
	void(*draw)(void);
#ifdef ANDROID
	struct android_app* app; 
#endif
}PGame;

int init_game();

void update_game();

int load_gamplay_code();

bool loaded_gamplay_library;
void* dynamic_lib_handle;
void(*loop_fuction_dynamic_loaded)(void*); 
void close_dynamic_game_play();


void pe_game_render_config();

void pe_game_window_init();

PGame* game;

#endif //PAVON_MOBILE_GAME_H
