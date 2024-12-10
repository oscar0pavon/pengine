#include <engine/engine.h>


void pe_input_thread() {

  for (;;) {
    pe_wm_events_update();
    pe_wm_input_update();
    pe_game_input();
  }
}

void pe_main_loop(void (*game_update)(void)) {

  pe_init();

  pe_wm_create_x11_window();
  pe_init_x11_keys();//after window creation

  pe_game_render_init(); // editor init
  pe_game_render_config();

  render_thread_init();

  pthread_t input_thread;
  pthread_create(&input_thread,NULL,&pe_input_thread,NULL);


  //Main loop 
  while (1) { //TODO: window should close
    
    game_update();

    time_start();

    pe_game_draw();

    if (pe_renderer_type == PEWMOPENGLES2) {
      pe_wm_swap_buffers();
    }

    time_end();

  }
}
