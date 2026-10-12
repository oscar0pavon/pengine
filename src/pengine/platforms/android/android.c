#include "android.h"
#include <android/input.h>
#include <pengine/windows_manager.h>
#include <pengine/game.h>
#include <pengine/log.h>
#include <pengine/input.h>


void pe_android_handle_cmd(struct android_app *app, int32_t cmd) {
  //LOG("##### Android CMD ######");
  switch (cmd) {
  case APP_CMD_SAVE_STATE:
    break;
  case APP_CMD_INIT_WINDOW:
    //pe_wm_egl_init();
    //pe_wm_swap_buffers();

    //pe_game_create(game);
    LOG("############## android windows init  #############");
    pe_is_window_init = true;
    break;
  case APP_CMD_TERM_WINDOW:

    LOG("##### Android TERM WINDOW ######");
    pe_wm_egl_end();
    pe_is_window_terminate = true; 
    break;
  case APP_CMD_LOST_FOCUS:
    
    LOG("##### Android LOST FOCUS ######");
    pe_wm_swap_buffers();
    break;
  }
}

void pe_android_poll_envents(){
    struct android_app* state = game->app;
    int ident;
    int events;
    struct android_poll_source *source;

    while ((ident = ALooper_pollAll(0, NULL, &events, (void **)&source)) >= 0) {

      // Process this event.
      if (source != NULL) {
        source->process(state, source);
      }

      // Check if we are exiting.
      if (state->destroyRequested != 0) {
				pe_wm_egl_end();	
        return;
      }
    }
}
