#include "editor.h"


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


  pe_renderer_type = PEWMOPENGLES2;
  pe_game_create(game);

}
