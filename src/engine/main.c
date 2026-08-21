#include "engine/window_manager.h"
#include <engine/base.h>
#include <engine/engine.h>


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

void pe_main_loop() {

  pe_init();

  pe_render_thread_init();

  game->init();


  pthread_t input_thread;
  pthread_create(&input_thread,NULL,&pe_input_thread,NULL);


  pengine_initialized = true;

  //Main loop 
  while (1) { //TODO: window should close
    
    game->update();

    start_delta_time();//frame time

    pe_frame_draw();

    game->draw();

    update_delta_time();

  }
}
