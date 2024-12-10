#include "editor_mode.h"

#include "../engine/game.h"
#include "../engine/camera.h"
#include "engine/input.h"

void change_to_editor_sub_mode(EditorMode mode) {
  pe_input_clean();
  switch (editor_mode) {
  case EDITOR_DEFAULT_MODE:

    break;
  case EDITOR_MODE_GUI_EDITOR:

    break;
  default:
    if (editor_mode != EDITOR_DEFAULT_MODE && mode == EDITOR_SUB_MODE_GRAB) {
      change_to_editor_mode(EDITOR_DEFAULT_MODE);
      return;
    }
    break;
  }

  if (editor_sub_mode == mode && mode != EDITOR_SUB_MODE_NULL) {
    change_to_editor_sub_mode(EDITOR_SUB_MODE_NULL);
    return;
  }

  switch (mode) {
  case EDITOR_SUB_MODE_TEXT_INPUT:
    strcpy(editor_sub_mode_text, "Text Input");
    editor_sub_mode = mode;
    break;
  case EDITOR_SUB_MODE_GRAB:
    strcpy(editor_sub_mode_text, "Grab");
    editor_sub_mode = mode;
    break;
  case EDITOR_SUB_MODE_SCALE:
    strcpy(editor_sub_mode_text, "Scale");
    editor_sub_mode = mode;
    break;
  case EDITOR_SUB_MODE_ROTATE:
    strcpy(editor_sub_mode_text, "Rotate");
    editor_sub_mode = mode;
    break;
  case EDITOR_SUB_MODE_NULL:
    strcpy(editor_sub_mode_text, "Not submode");
    editor_sub_mode = mode;
    break;
  }
}

void change_to_editor_mode(EditorMode mode) {
  pe_input_clean();

  if (editor_sub_mode != EDITOR_SUB_MODE_NULL){
    change_to_editor_sub_mode(EDITOR_SUB_MODE_NULL);
  }

  if (editor_mode == mode) {
    editor_mode = EDITOR_CHANGING_MODE_MODE;
    change_to_editor_mode(EDITOR_DEFAULT_MODE);
    return;
  }
  editor_mode = EDITOR_CHANGING_MODE_MODE;
  mode_to_change = mode;
  switch (mode) {
  case EDITOR_DEFAULT_MODE:
    strcpy(editor_mode_show_text, "Default");
    break;
  case PE_EDITOR_MODELING_MODE:
    strcpy(editor_mode_show_text, "Modeling");
    break;
  case EDITOR_NAVIGATE_MODE:
    strcpy(editor_mode_show_text, "Navigate");
    move_camera_input = true;
    break;
  case EDITOR_GRAB_MODE:
    strcpy(editor_mode_show_text, "Grab");
    LOG("## Grab mode");
    break;
  case EDITOR_ROTATE_MODE:
    strcpy(editor_mode_show_text, "Rotate");
    break;
  case EDITOR_MODE_GUI_EDITOR:
    strcpy(editor_mode_show_text, "GUI");
    break;
    break;
  case EDITOR_PLAY_MODE: {
    if (game_initialized == false) {
      if (init_game() == -1) {
        change_to_editor_mode(EDITOR_DEFAULT_MODE);
        return;
      }
      game_initialized = true;
    }
    if (game_initialized == false) {
      change_to_editor_mode(EDITOR_DEFAULT_MODE);
      return;
    }
    game_initialized = true;
    strcpy(editor_mode_show_text, "Rotate");
    if (loaded_gamplay_library == false) {
      load_gamplay_code();
      loaded_gamplay_library = true;
    }
    break;
  }
  case EDITOR_CHANGING_MODE_MODE:
    editor_mode = mode_to_change;
    break;
  default:
    break;
  }
  pe_input_clean();
}
