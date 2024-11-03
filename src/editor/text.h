#ifndef EDITOR_TEXT_H
#define EDITOR_TEXT_H

#include <engine/text_renderer.h>
#include <editor/editor_mode.h>

#include <stdbool.h>
#include <engine/array.h>

#include "input.h"

#define DIRECTORY_MODELS 0
#define DIRECTORY_TEXTURES 1

typedef enum{
    MENU_TYPE_ADD_MODEL,
    MENU_TYPE_ADD_TEXTURE,
}PETextMenuType;

typedef struct PETextMenu PETextMenu;

typedef void(*PETextMenuFunction)(PETextMenu*);

typedef struct PETextMenu{
    char name[30];
    bool show;
    bool element_selected;
    bool execute;
		bool always_open;
		bool menu_in_editor;
		EditorMode editor_mode;
    unsigned int actual_element_select;
    PETextMenuFunction execute_function;
    PETextMenuFunction draw_text_funtion;
    PETextMenuType type;
    unsigned short int element_count;
    unsigned short int text_size;
    char text_for_action[30];
    Key* open_key;
    int mods_key;
}PETextMenu;

void draw_directory_files();
void draw_directory_file_type(unsigned short int type);


void draw_element_text_list(PETextMenu *menu, const char *text, int i);

void text_renderer_loop();

void pe_editor_draw_command_line();

int mark_id;

void editor_text_init();

#endif
