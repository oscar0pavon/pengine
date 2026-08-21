//
// Created by pavon on 6/24/19.
//

#include "game.h"
#include "input.h"

#include <cglm/cglm.h>

#include "camera.h"

#include "engine.h"
#include "window_manager.h"

#include <dlfcn.h>
#include <engine/base.h>
#include <engine/input.h>

static vec4 backcolor= {0.1,0.2,0.4,1};


void pe_game_draw() {

  if (game->draw)
    game->draw();
}

void pe_game_create_window(){


}

void pe_game_input(){
  game->input();

}


int load_gamplay_code(){
    
    char *error;

    dynamic_lib_handle = dlopen("../Game/src/test.so", RTLD_GLOBAL | RTLD_NOW);
    if(!dynamic_lib_handle){
        LOG("ERROR: Gameplay library not loaded\nError: %s \n",dlerror() );
        return -1;
    }    

    loop_fuction_dynamic_loaded = dlsym(dynamic_lib_handle,"test");
    if ((error = dlerror()) != NULL) 
    {
        fprintf(stderr, "%s\n", error);
        return -1;
    }
  
    return 0;
}

void close_dynamic_game_play(){
    if(dynamic_lib_handle)
        dlclose(dynamic_lib_handle);
}


