#include "engine/window_manager.h"
#include <engine/base.h>
#include <engine/engine.h>
#include <engine/renderer/render_thread.h>
#include <stdio.h>

//INFO the signature pthread_create() wants, not a void() cast into place. the
//return value is never read - the loop does not end - but a thread entry with
//the wrong type is undefined behaviour, not a formality
void *pe_input_thread(void *argument) {

  for (;;) {
    pe_wm_events_update();
    pe_wm_input_update();
    pe_game_input();
  }

  return NULL;
}

void pengine_run(PGame* created_game){

  game = created_game; 

  pe_init();

  pe_create_window();

  pe_vk_init();

  game->init();


  pthread_t input_thread;
  pthread_create(&input_thread,NULL,&pe_input_thread,NULL);


  pengine_initialized = true;

  //Main loop 
  start_delta_time();
  while (pe_running) { //TODO: window should close
    
    //INFO the whole frame is timed, update included: measuring only the
    //draw made delta_time shorter than the real frame by whatever update
    //cost, so everything driven by it ran slow once update got heavy
    update_delta_time();

    game->update();

    pe_frame_draw();

  }

  LOG("end pengine\n");

  pe_end();

}
