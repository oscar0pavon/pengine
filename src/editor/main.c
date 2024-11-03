#include "editor.h"

int oldmain(int argc , char* argv[]){

  pe_renderer_type = PEWMOPENGLES2;
  command_parse_from_command_line(argc, argv);

  pe_editor_init();

  pe_program_main_loop(&editor_main_loop, window_editor_main);

  editor_finish();

}

void aloop(){

}

int main(int argc , char* argv[]){

  PGame editor;
  ZERO(editor);
  editor.name = "Editor";
  editor.init = &pe_editor_init; 
  editor.input = &editor_window_level_editor_input_update; // handle editor modes
  editor.loop = &aloop;
  editor.draw = &pe_editor_draw;
  game = &editor;


  pe_renderer_type = PEWMVULKAN;
  pe_game_create(game);

}
