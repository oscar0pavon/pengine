#include <pengine/engine.h>

#include <pengine/platforms/android/android.h>

#include <editor/modeling.h>

#include <pengine/../../demos/chess/chess.h>

#include <editor/editor.h>


void ainit() {

  camera_init(&main_camera);
  init_vec3(-30, 0, 0, main_camera.position);
  camera_update(&main_camera);

  pe_change_background_color(1, 0, 0, 1);

  init_modeling();
}

void draw(){

}

void aloop(){


}


void ainput(){
	
  

	if(key_released(&input.W)){
		vertex_new(0.01,0,0)	;
	}
	
  if(key_released(&input.Q)){
    exit(0);
  }


}

void android_main(struct android_app* android_state) {


  PGame modelling;
  ZERO(modelling);
  modelling.name = "Modelling";
  modelling.init = &ainit;
  modelling.input = &ainput;
  modelling.loop = &aloop;
  modelling.draw = &draw;
	modelling.app = android_state;
	game = &modelling;	//need for egl context creation

  PGame editor;
  ZERO(editor);
  editor.name = "Editor";
  editor.init = &pe_editor_init; 
  editor.input = &editor_window_level_editor_input_update; // handle editor modes
  editor.loop = &aloop;
  editor.draw = &pe_editor_draw;
  editor.app = android_state;
  game = &editor;

  PGame chess_game;
  ZERO(chess_game);
  chess_game.app = android_state;
//  PGame* chess = chess_main(&chess_game);

  // TODO: game reference on android devices need to be create for input
  // and handle
  game->app->onAppCmd = &pe_android_handle_cmd;
  game->app->onInputEvent = &pe_android_input_handle;


  pe_game_create(game);

	//pe_log("hello world, from android read file");

}
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
