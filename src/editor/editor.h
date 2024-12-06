#ifndef EDITOR_EDITOR
#define EDITOR_EDITOR

#include "../ThirdParty/cglm/cglm.h"

#include "../engine/engine.h"
#include "data.h"

#include "input.h"

#include "editor_shader.h"

#include "gizmos.h"

#include "export_model.h"


#ifdef DESKTOP


#include "content_manager.h"

#endif


#include "editor_command_queue.h"

#include "stats.h"

#include "level.h"

#include "commands.h"

#include "network.h"

typedef struct HLODCluster HLODCluster;

void pe_editor_init();

void pe_editor_draw();

void editor_main_loop();

void editor_update();

void get_element_status(Element* element);

void editor_load_level(const char*);

void deselect_all();

void editor_finish();

void rotate_editor_element(Element* element, float angle, vec3 axis);

void rotate_editor_selected_element_with_quaternion(versor quaternion);

void remove_selected_element();

void reload_editor();

void duplicate_selected_element(int,Element*);

void add_editor_native_element(const char* native_element_name);

void play_game_standalone();

void editor_add_HLOD_element(HLODCluster*);

void editor_generate_and_add_cube_element();

void editor_focus_selected_element();

void editor_add_element_with_model_path(const char*);

void editor_init_new_added_element();
//++++++++++++++++++++++
//Global variables
char editor_mode_show_text[60];
char editor_sub_mode_text[60];

float camera_velocity;

Array editor_elements;

Array selected_elements_id;

char opened_file_name[20];

bool controlling_camera_component;

bool isDrawUV;

bool editor_search_objects;

PTexture editor_texture_checker;

bool editor_running;

static vec4 editor_background_color = {0.1,0.2,0.4,1};

bool editor_file_explorer_show;
bool editor_content_browser_show;
bool editor_content_browser_initiliazed;
bool editor_content_browser_updated;

Array editor_models;
Array editor_textures;

Array LOD_models;

#endif
