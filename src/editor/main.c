#include "editor.h"


void update(){

}

int main(int argc , char* argv[]){

  PGame editor;
  ZERO(editor);
  editor.name = "Editor";
  editor.init = &pe_editor_init; 
  editor.input = &editor_window_level_editor_input_update; // handle editor modes
  editor.update = &update;
  editor.draw = &pe_editor_draw;

  pe_renderer_type = PEWMOPENGLES2;

  pengine_run(&editor);

}
