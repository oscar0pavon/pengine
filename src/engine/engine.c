//
// Created by pavon on 6/24/19.
//

#include "engine.h"
#include <unistd.h>

#include "game.h"
#include "images.h"

#include "model.h"

#include "physics.h"
#include "window_manager.h"

#include <engine/animation/animation.h>

#include <engine/renderer/render_thread.h>

#include <engine/base.h>

#include "time.h"

Array engine_textures;

void pe_debug_print_mat4(mat4 mat) {

  LOG("matrix0 %f %f %f %f\n", mat[0][0], mat[0][1], mat[0][2], mat[0][3]);
  LOG("matrix1 %f %f %f %f\n", mat[1][0], mat[1][1], mat[1][2], mat[1][3]);
  LOG("matrix2 %f %f %f %f\n", mat[2][0], mat[2][1], mat[2][2], mat[2][3]);
  LOG("matrix3 %f %f %f %f\n", mat[3][0], mat[3][1], mat[3][2], mat[3][3]);
}

void pe_input_character(unsigned int codepoint) {
  if (codepoint == 241) // equal "ñ"
    return;
 
  char keyboard_utf = (char)codepoint;

#ifdef ANDROID
  pe_editor_parse_cmd_char(keyboard_utf);
#endif
}

void update_mvp(mat4 model, mat4 mvp_out) {
  mat4 projection_view;
  glm_mul(main_camera.projection, main_camera.view, projection_view);
  glm_mul(projection_view, model, mvp_out);
}

void pe_change_background_color(float r, float g, float b, float a) {

  vec4 color = {r, g, b, a};
  glm_vec4_copy(color, pe_background_color);
}

void new_empty_model_in_array(Array *array) {
  PModel new_model;
  memset(&new_model, 0, sizeof(PModel));
  array_add(array, &new_model);

  selected_model = array_get(array, array->count - 1);

  selected_model->id = array->count - 1;
}

void new_empty_model() {
  PModel new_model;
  ZERO(new_model);
  if (!actual_model_array)
    return;
  glm_mat4_identity(new_model.model_mat);
  array_add(actual_model_array, &new_model);

  selected_model = array_get(actual_model_array, actual_model_array->count - 1);

  selected_model->id = actual_model_array->count - 1;
}

void load_simple_image(const char *path) {

  PTexture new_texture;
  ZERO(new_texture);
  array_add(current_textures_array, &new_texture);

  PTexture *texture_loaded =
      array_get(current_textures_array, current_textures_array->count - 1);
  if (pe_load_texture(path, texture_loaded) == -1)
    return;
}

void add_action_function(void (*f)(void)) {
  ActionPointer new_action;
  new_action.id = action_pointer_id_count;
  new_action.action = f;
  array_add(&actions_pointers, &new_action);
  action_pointer_id_count++;
}

void duplicate_model_data(PModel *destination, PModel *source) {
  memcpy(destination, source, sizeof(PModel));
}

void pe_init_arrays() {
  array_init(&pe_arr_models_paths, sizeof(char[100]), 50);
  array_init(&pe_arr_tex_paths, sizeof(char[20]), 50);

  array_init(&array_models_loaded, sizeof(PModel), 100);

  array_init(&actions_pointers, sizeof(ActionPointer), 20);

  array_init(&array_animation_play_list, sizeof(PEAnimationPlay), 100);

  array_init(&array_render_thread_init_commmands, sizeof(ExecuteCommand), 5);

  array_init(&array_render_thread_commands, sizeof(ExecuteCommand), 100);

  array_init(&render_thread_commads, sizeof(PEThreadCommand), 100);

  array_init(&pe_array_textures, sizeof(PTexture), 100);

  current_textures_array = &pe_array_textures;
  actual_model_array = &array_models_loaded;

  touch_position_x = -1;
  touch_position_x = -1;

  action_pointer_id_count = 0;

  pe_is_window_init = false;

  pe_is_window_terminate = false;
}

void pe_init_global_variables() {

  pe_data_loader_models_loaded_count = 0;

  engine_running = true;

}


void pengine_run(PGame* created_game){
    game = created_game; 

    pe_main_loop();

}
