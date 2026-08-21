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

#include <cglm/cglm.h>
#include "model.h"
#include "types.h"

#include "file_loader.h"
#include "memory.h"

#include "time.h"

#include "animation/animation.h"

#include "camera.h"

#include "renderer/renderer.h"

#include "Collision/collision.h"

#ifdef DESKTOP
#include "content_manager.h"

#include "audio/audio_engine.h"
#endif

#include "game.h"

#include "threads.h"

#define VEC3(p1, p2, p3)                                                       \
  (vec3) { p1, p2, p3 }
#define VEC4(p1, p2, p3, p4)                                                   \
  (vec4) { p1, p2, p3, p4 }

#define VEC2(p1, p2)                                                           \
  (vec2) { p1, p2 }

#define COLOR(color) color[0], color[1], color[2], color[3]

void pe_debug_print_mat4(mat4 mat);

void pe_input_character(unsigned int codepoint);

void pe_init_global_variables();

void pe_change_background_color(float r, float g, float b, float a);

void add_action_function(void (*f)(void));

/*Create new model in actual model array and you can use selected_model after */
void new_empty_model();

void new_empty_model_in_array(Array *array);

void duplicate_model_data(PModel *destination, PModel *source);

void load_simple_image(const char *path);

void pe_frame_clean();

void pe_frame_draw();

void pe_init_arrays();

void update_mvp(mat4 model, mat4 mvp_out);


void pengine_run(PGame*);
void pe_main_loop();
//
// Global variables
//

bool engine_running;

bool pengine_initialized;

bool should_close;

float frame_time;

bool game_initialized;

int action_pointer_id_count;

u32 FPS;
//
// Global array containers
//
Array array_models_loaded;

Array pe_array_textures;

Array actions_pointers;

//
// Global pointers
//
PModel *selected_model;

Array *current_textures_array;
Array *actual_model_array;

//
// Paths data
//
Array pe_arr_models_paths;
Array pe_arr_tex_paths;

// ThreadsCommads
Array render_thread_commads;
Array main_thread_commads;

vec4 pe_background_color;

#endif // PAVON_ENGINE_H
