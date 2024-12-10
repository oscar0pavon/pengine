//
// Created by pavon on 6/24/19.
//

#ifndef PAVON_ENGINE_H
#define PAVON_ENGINE_H

#include "array.h"
#include "interruptions.h"
#include "log.h"
#include "threads.h"

#include "input.h"

#include "vertex.h"

#include "../ThirdParty/cglm/cglm.h"
#include "model.h"
#include "types.h"

#include "file_loader.h"
#include "files_parser.h"
#include "gui.h"
#include "memory.h"

#include "time.h"

#include "shader.h"

#include "components/components.h"

#include "animation/animation.h"

#include "camera.h"

#include "renderer/renderer.h"

#include "Collision/collision.h"

#ifdef DESKTOP
#include "content_manager.h"

#include "text_renderer.h"

#include "audio/audio_engine.h"
#endif

#include "game.h"

#include "threads.h"

#include "elements.h"

#define VEC3(p1, p2, p3)                                                       \
  (vec3) { p1, p2, p3 }
#define VEC4(p1, p2, p3, p4)                                                   \
  (vec4) { p1, p2, p3, p4 }

#define VEC2(p1, p2)                                                           \
  (vec2) { p1, p2 }

#define COLOR(color) color[0], color[1], color[2], color[3]

static const char *const level_folder = "../assets/Game/levels/";
static const char *const gui_folder = "../assets/gui/";

void pe_debug_print_mat4(mat4 mat);

void pe_input_character(unsigned int codepoint);

void pe_init_global_variables();

void pe_element_rotate(Element *element, float angle, vec3 axis);

void pe_change_background_color(float r, float g, float b, float a);

void engine_loop();

void init_game_engine();

void add_action_function(void (*f)(void));

/* Draw array of engine elements,
if in editor the shader need color multiplication uniform otherwise
it not show it because color is multiplied per 0 ! */
void engine_draw_elements(Array *elements);
void pe_render_skinned(Array *elements);
void draw_simgle_model(PModel *new_model);

void draw_editor_viewport();

/*Create new model in actual model array and you can use selected_model after */
void new_empty_model();

void add_texture_to_selected_element_with_image_path(const char *image_path);

void pe_tex_loaded_to_model(int id);

void load_model_to_array(Array *array, const char *path_model,
                         const char *color_texture_path);

int load_and_initialize_simple_model(const char *model_gltf_path);

void pe_program_main_loop(void (*program_loop)(void));

// Transform
void update_translation(vec3 translation);
void update_scale(vec3 translation);
//*******

void load_simple_image(const char *path);

void test_elements_occlusion();

void duplicate_model_data(PModel *destination, PModel *source);

void new_empty_model_in_array(Array *array);

void check_meshes_distance();

void engine_add_texture_from_memory_to_selected_element(void *data, u32 size);

void pe_frame_clean();

void pe_frame_draw();

void pe_init_arrays();

void update_mvp(mat4 model, mat4 mvp_out);

//
// Global variables
//

bool engine_running;

bool engine_initialized;

bool should_close;

float frame_time;

bool game_initialized;

unsigned int element_id_count;

unsigned int components_id_count;

ComponentType current_loaded_component_type;

int action_pointer_id_count;

u32 FPS;
//
// Global array containers
//
Array components;
Array engine_native_models;

Array array_models_loaded;
Array pe_arr_skin_loaded;
Array array_hirarchical_level_of_detail;

Array pe_array_textures;

Array actions_pointers;
;

//
// Global pointers
//
Element *selected_element;
PModel *selected_model;
Element *player1;
Element *player_start;

ComponentDefinition *current_component_selected;

Array *previous_models_array;
Array *current_textures_array;
Array *actual_model_array;
Array *actual_elements_array;

Array *actual_buttons_array;

//
/*Draw frame data */
//
Array frame_draw_static_elements;
Array frame_draw_skinned_elements;
Array models_for_test_occlusion;
Array array_static_meshes_pointers;
Array array_static_meshes_pointers_for_test_distance;
Array array_skinned_mesh_pointers;
Array array_skinned_mesh_for_distance_test;

//
// Paths data
//
Array pe_arr_models_paths;
Array pe_arr_tex_paths;

// ThreadsCommads
Array render_thread_commads;
Array main_thread_commads;

vec4 pe_background_color;

PModel *test_model;
PModel *test_model2;
PModel *anim_model;

#endif // PAVON_ENGINE_H
