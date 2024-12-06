#ifndef EDITOR_WINDOW_CONTENT_H
#define EDITOR_WINDOW_CONTENT_H


#include <engine/window_manager.h>

#include "../../engine/content_manager.h"

struct Hint{
    char keys[4];
    u8 key_count;
};

struct ContentViewPort{
    int max_x;
    int last_x;
    int last_y;
    int object_x_count;
};

typedef struct ContentView{
	ContentType type;
    DrawData draw;    
    vec2 position;
    vec2 size;
    bool selected;
    bool new_update;
    u8 pixel_size;//thumbnail + text size in pixel
    u8 text_size;
    u8 thumbnail_image_size;
    char thumbnail_image_path[100];
    char content_name[100];   
}ContentView;

void editor_window_content_init();

void editor_window_content_browser_draw(); 
void editor_window_content_browser_input_update();

void content_create_thumbnail(const char * brute_content_path,ContentType type);

void editor_window_content_browser_update();

void editor_window_content_browser_close_window();

EngineWindow* window_content_browser;

bool editor_window_content_browser_hint;

#endif
