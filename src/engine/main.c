#include <engine/engine.h>


void pe_input_thread() {

  for (;;) {
    pe_wm_events_update();
    pe_wm_input_update();
    pe_game_input();
  }
}

void pe_main_loop() {

  pe_init();

  pe_wm_create_x11_window();
  pe_init_x11_keys();//after window creation
  
  pe_render_thread_init();

  game->init();


  pthread_t input_thread;
  pthread_create(&input_thread,NULL,&pe_input_thread,NULL);


  pengine_initialized = true;

  //Main loop 
  while (1) { //TODO: window should close
    
    game->update();

    time_start();//frame time

    pe_frame_draw();

    game->draw();

    if (pe_renderer_type == PEWMOPENGLES2) {
      pe_wm_swap_buffers();
    }

    time_end();

  }
}
